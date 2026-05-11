#include "Editor.h"
#include "imgui.h"
#include "../scene/Scene.h"
#include "../scene/Object.h"

void Editor::draw(Scene& scene) {
    drawSceneHierarchy(scene);
    drawInspector(scene);
}

static float distPointToSegment(ImVec2 p, ImVec2 a, ImVec2 b) {
    ImVec2 ab = { b.x - a.x, b.y - a.y };
    ImVec2 ap = { p.x - a.x, p.y - a.y };
    float lenSq = ab.x * ab.x + ab.y * ab.y;
    if (lenSq < 1e-6f) {
        float dx = p.x - a.x, dy = p.y - a.y;
        return std::sqrtf(dx * dx + dy * dy);
    }
    float t = (ap.x * ab.x + ap.y * ab.y) / lenSq;
    t = std::clamp(t, 0.0f, 1.0f);
    ImVec2 proj = { a.x + ab.x * t, a.y + ab.y * t };
    float dx = p.x - proj.x, dy = p.y - proj.y;
    return std::sqrtf(dx * dx + dy * dy);
}

void Editor::updateGizmoHover(Scene& scene, int mouseX, int mouseY,
    int screenW, int screenH, Object* cameraObj)
{
    if (activeGizmoAxis != GizmoAxis::None) return;

    hoveredGizmoAxis = (gizmoMode == GizmoMode::Rotate)
        ? pickRotateGizmo(scene, mouseX, mouseY, screenW, screenH, cameraObj)
        : pickGizmoAxis(scene, mouseX, mouseY, screenW, screenH, cameraObj);
}

GizmoAxis Editor::pickRotateGizmo(Scene& scene, int mouseX, int mouseY,
    int screenW, int screenH, Object* cameraObj)
{
    if (selectedObjectIndex < 0) return GizmoAxis::None;
    Object* obj = scene.objects[selectedObjectIndex].get();
    Camera* cam = cameraObj->getComponent<Camera>();
    if (!cam) return GizmoAxis::None;

    Mat4 view = cam->viewMatrix(cameraObj->transform);
    Vec3 origin = obj->transform.position;
    const float radius = 1.0f;
    const int segments = 48;

    auto project = [&](Vec3 w, ImVec2& out) -> bool {
        Vec3 cs = view * w;
        ScreenVert sv;
        if (!cam->project(cs, screenW, screenH, sv)) return false;
        out = ImVec2(sv.x, sv.y);
        return true;
        };

    auto distToCircle = [&](Vec3 axis) -> float {
        Vec3 u, v;
        if (std::abs(axis.x) < 0.9f) u = normalize(cross(axis, Vec3{ 1, 0, 0 }));
        else                          u = normalize(cross(axis, Vec3{ 0, 1, 0 }));
        v = normalize(cross(axis, u));

        ImVec2 mouse{ (float)mouseX, (float)mouseY };
        float bestDist = std::numeric_limits<float>::max();

        ImVec2 prev;
        bool hasPrev = false;
        for (int i = 0; i <= segments; i++) {
            float a = (i / (float)segments) * 2.0f * 3.14159265f;
            Vec3 p = origin + u * (std::cosf(a) * radius) + v * (std::sinf(a) * radius);
            ImVec2 sp;
            if (!project(p, sp)) { hasPrev = false; continue; }
            if (hasPrev) {
                float d = distPointToSegment(mouse, prev, sp);
                if (d < bestDist) bestDist = d;
            }
            prev = sp;
            hasPrev = true;
        }
        return bestDist;
        };

    const float hitRadius = 8.0f;
    float dx = distToCircle({ 1, 0, 0 });
    float dy = distToCircle({ 0, 1, 0 });
    float dz = distToCircle({ 0, 0, 1 });

    float best = hitRadius;
    GizmoAxis hit = GizmoAxis::None;
    if (dx < best) { best = dx; hit = GizmoAxis::X; }
    if (dy < best) { best = dy; hit = GizmoAxis::Y; }
    if (dz < best) { best = dz; hit = GizmoAxis::Z; }
    return hit;
}

GizmoAxis Editor::pickGizmoAxis(Scene& scene, int mouseX, int mouseY,
    int screenW, int screenH, Object* cameraObj)
{
    if (selectedObjectIndex < 0) return GizmoAxis::None;
    Object* obj = scene.objects[selectedObjectIndex].get();
    Camera* cam = cameraObj->getComponent<Camera>();
    if (!cam) return GizmoAxis::None;

    Mat4 view = cam->viewMatrix(cameraObj->transform);

    auto project = [&](Vec3 w, ImVec2& out) -> bool {
        Vec3 cs = view * w;
        ScreenVert sv;
        if (!cam->project(cs, screenW, screenH, sv)) return false;
        out = ImVec2(sv.x, sv.y);
        return true;
        };

    ImVec2 sOrigin, sX, sY, sZ;
    if (!project(obj->transform.position, sOrigin)) return GizmoAxis::None;
    project(obj->transform.position + Vec3{ 1,0,0 }, sX);
    project(obj->transform.position + Vec3{ 0,1,0 }, sY);
    project(obj->transform.position + Vec3{ 0,0,1 }, sZ);

    ImVec2 mouse{ (float)mouseX, (float)mouseY };
    const float hitRadius = 8.0f;

    float dx = distPointToSegment(mouse, sOrigin, sX);
    float dy = distPointToSegment(mouse, sOrigin, sY);
    float dz = distPointToSegment(mouse, sOrigin, sZ);

    float best = hitRadius;
    GizmoAxis hit = GizmoAxis::None;
    if (dx < best) { best = dx; hit = GizmoAxis::X; }
    if (dy < best) { best = dy; hit = GizmoAxis::Y; }
    if (dz < best) { best = dz; hit = GizmoAxis::Z; }
    return hit;
}

void Editor::updateGizmoRotateDrag(Scene& scene, int mouseX, int mouseY,
    int screenW, int screenH, Object* cameraObj)
{
    if (activeGizmoAxis == GizmoAxis::None) return;
    if (selectedObjectIndex < 0) { activeGizmoAxis = GizmoAxis::None; return; }

    Object* obj = scene.objects[selectedObjectIndex].get();
    Camera* cam = cameraObj->getComponent<Camera>();
    if (!cam) return;
    Mat4 view = cam->viewMatrix(cameraObj->transform);

    auto project = [&](Vec3 w, ImVec2& out) -> bool {
        Vec3 cs = view * w;
        ScreenVert sv;
        if (!cam->project(cs, screenW, screenH, sv)) return false;
        out = ImVec2(sv.x, sv.y);
        return true;
        };

    ImVec2 sOrigin;
    if (!project(obj->transform.position, sOrigin)) return;

    // vector from object center on screen to the mouse position
    ImVec2 toMouse = {
        gizmoLastMousePos.x - sOrigin.x,
        gizmoLastMousePos.y - sOrigin.y
    };
    float toMouseLen = std::sqrtf(toMouse.x * toMouse.x + toMouse.y * toMouse.y);
    if (toMouseLen < 1e-4f) return;

    // tangent direction (perpendicular to toMouse, rotated 90° CCW)
    ImVec2 tangent = { -toMouse.y / toMouseLen, toMouse.x / toMouseLen };

    ImVec2 mouseDelta = {
        (float)mouseX - gizmoLastMousePos.x,
        (float)mouseY - gizmoLastMousePos.y
    };

    // how much the mouse moved tangentially -> angular delta
    float dotMD = mouseDelta.x * tangent.x + mouseDelta.y * tangent.y;
    const float radiansPerPixel = 0.01f;
    float deltaAngle = dotMD * radiansPerPixel;

    Vec3 axisWorld;
    switch (activeGizmoAxis) {
		case GizmoAxis::X: axisWorld = { 1, 0, 0 }; break;
		case GizmoAxis::Y: axisWorld = { 0, 1, 0 }; break;
		case GizmoAxis::Z: axisWorld = { 0, 0, 1 }; break;
		default: return;
    }

    // vector from object to camera, in world space
    Vec3 toCam = normalize(cameraObj->transform.position - obj->transform.position);
    float facing = dot(axisWorld, toCam);
    if (facing < 0) deltaAngle = -deltaAngle;

    switch (activeGizmoAxis) {
		case GizmoAxis::X: obj->transform.pitch += deltaAngle; break;
        case GizmoAxis::Y: obj->transform.yaw -= deltaAngle; break;
		case GizmoAxis::Z: obj->transform.roll += deltaAngle; break;
		default: break;
    }
    gizmoLastMousePos = ImVec2((float)mouseX, (float)mouseY);
}

void Editor::updateGizmoScaleDrag(Scene& scene, int mouseX, int mouseY,
    int screenW, int screenH, Object* cameraObj)
{
    if (activeGizmoAxis == GizmoAxis::None) return;
    if (selectedObjectIndex < 0) { activeGizmoAxis = GizmoAxis::None; return; }

    Object* obj = scene.objects[selectedObjectIndex].get();
    Camera* cam = cameraObj->getComponent<Camera>();
    if (!cam) return;
    Mat4 view = cam->viewMatrix(cameraObj->transform);

    Vec3 axisWorld;
    switch (activeGizmoAxis) {
    case GizmoAxis::X: axisWorld = { 1,0,0 }; break;
    case GizmoAxis::Y: axisWorld = { 0,1,0 }; break;
    case GizmoAxis::Z: axisWorld = { 0,0,1 }; break;
    default: return;
    }

    // Project origin and origin+axis to screen
    auto project = [&](Vec3 w, ImVec2& out) -> bool {
        Vec3 cs = view * w;
        ScreenVert sv;
        if (!cam->project(cs, screenW, screenH, sv)) return false;
        out = ImVec2(sv.x, sv.y);
        return true;
        };

    ImVec2 sOrigin, sEnd;
    if (!project(obj->transform.position, sOrigin)) return;
    if (!project(obj->transform.position + axisWorld, sEnd)) return;

    ImVec2 screenAxis = { sEnd.x - sOrigin.x, sEnd.y - sOrigin.y };
    float lenSq = screenAxis.x * screenAxis.x + screenAxis.y * screenAxis.y;
    if (lenSq < 1e-4f) return;

    ImVec2 mouseDelta = {
        (float)mouseX - gizmoLastMousePos.x,
        (float)mouseY - gizmoLastMousePos.y
    };

    // amount the mouse moved along the screen-space axis,
    // normalized to "world units per pixel along that axis"
    float dotMD = mouseDelta.x * screenAxis.x + mouseDelta.y * screenAxis.y;
    float deltaWorld = dotMD / lenSq;

    obj->transform.scale = obj->transform.scale + axisWorld * deltaWorld;
	obj->transform.scale.x = std::max(0.1f, obj->transform.scale.x);
	obj->transform.scale.y = std::max(0.1f, obj->transform.scale.y);
	obj->transform.scale.z = std::max(0.1f, obj->transform.scale.z);

    gizmoLastMousePos = ImVec2((float)mouseX, (float)mouseY);
}

void Editor::updateGizmoTranslateDrag(Scene& scene, int mouseX, int mouseY,
    int screenW, int screenH, Object* cameraObj)
{
    if (activeGizmoAxis == GizmoAxis::None) return;
    if (selectedObjectIndex < 0) { activeGizmoAxis = GizmoAxis::None; return; }

    Object* obj = scene.objects[selectedObjectIndex].get();
    Camera* cam = cameraObj->getComponent<Camera>();
    if (!cam) return;
    Mat4 view = cam->viewMatrix(cameraObj->transform);

    Vec3 axisWorld;
    switch (activeGizmoAxis) {
    case GizmoAxis::X: axisWorld = { 1,0,0 }; break;
    case GizmoAxis::Y: axisWorld = { 0,1,0 }; break;
    case GizmoAxis::Z: axisWorld = { 0,0,1 }; break;
    default: return;
    }

    // Project origin and origin+axis to screen
    auto project = [&](Vec3 w, ImVec2& out) -> bool {
        Vec3 cs = view * w;
        ScreenVert sv;
        if (!cam->project(cs, screenW, screenH, sv)) return false;
        out = ImVec2(sv.x, sv.y);
        return true;
        };

    ImVec2 sOrigin, sEnd;
    if (!project(obj->transform.position, sOrigin)) return;
    if (!project(obj->transform.position + axisWorld, sEnd)) return;

    ImVec2 screenAxis = { sEnd.x - sOrigin.x, sEnd.y - sOrigin.y };
    float lenSq = screenAxis.x * screenAxis.x + screenAxis.y * screenAxis.y;
    if (lenSq < 1e-4f) return;

    ImVec2 mouseDelta = {
        (float)mouseX - gizmoLastMousePos.x,
        (float)mouseY - gizmoLastMousePos.y
    };

    // amount the mouse moved along the screen-space axis,
    // normalized to "world units per pixel along that axis"
    float dotMD = mouseDelta.x * screenAxis.x + mouseDelta.y * screenAxis.y;
    float deltaWorld = dotMD / lenSq;

    obj->transform.position = obj->transform.position + axisWorld * deltaWorld;

    gizmoLastMousePos = ImVec2((float)mouseX, (float)mouseY);
}

static float rayAABB(Vec3 rayOrigin, Vec3 rayDir, Vec3 boxMin, Vec3 boxMax) {
    float tMin = 0.0f;
    float tMax = std::numeric_limits<float>::max();
    for (int i = 0; i < 3; i++) {
        float ro = (&rayOrigin.x)[i];
        float rd = (&rayDir.x)[i];
        float bmin = (&boxMin.x)[i];
        float bmax = (&boxMax.x)[i];
        if (std::abs(rd) < 1e-8f) {
            if (ro < bmin || ro > bmax) return -1.0f;
        }
        else {
            float t1 = (bmin - ro) / rd;
            float t2 = (bmax - ro) / rd;
            if (t1 > t2) std::swap(t1, t2);
            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);
            if (tMin > tMax) return -1.0f;
        }
    }
    return tMin;
}

void Editor::handleMouseClick(Scene& scene, int mouseX, int mouseY,
    int screenW, int screenH, Object* cameraObj)
{
    Camera* cam = cameraObj->getComponent<Camera>();
    if (!cam) return;
    Mat4 view = cam->viewMatrix(cameraObj->transform);

    int bestHit = -1;
    float bestDist = std::numeric_limits<float>::max();

    for (size_t i = 0; i < scene.objects.size(); i++) {
        Object* obj = scene.objects[i].get();
        auto* bc = obj->getComponent<BoxCollider>();
        if (!bc) continue;

        // Project center to screen
        Vec3 camSpace = view * obj->transform.position;
        ScreenVert sv;
        if (!cam->project(camSpace, screenW, screenH, sv)) continue;

        // Project corners of AABB to get approximate screen rect
        float minSX = FLT_MAX, maxSX = -FLT_MAX, minSY = FLT_MAX, maxSY = -FLT_MAX;
        for (int dx = -1; dx <= 1; dx += 2)
            for (int dy = -1; dy <= 1; dy += 2)
                for (int dz = -1; dz <= 1; dz += 2) {
                    Vec3 corner = obj->transform.position + Vec3{
                        bc->halfExtents.x * dx,
                        bc->halfExtents.y * dy,
                        bc->halfExtents.z * dz
                    };
                    Vec3 cs = view * corner;
                    ScreenVert cv;
                    if (cam->project(cs, screenW, screenH, cv)) {
                        minSX = std::min(minSX, cv.x);
                        maxSX = std::max(maxSX, cv.x);
                        minSY = std::min(minSY, cv.y);
                        maxSY = std::max(maxSY, cv.y);
                    }
                }

        if (mouseX >= minSX && mouseX <= maxSX && mouseY >= minSY && mouseY <= maxSY && camSpace.z < bestDist) {
            bestDist = camSpace.z;
            bestHit = (int)i;
        }
    }

    if (bestHit >= 0) selectedObjectIndex = bestHit;
	else selectedObjectIndex = -1;
}

static void drawCircle(ImDrawList* draw, const Mat4& view, const Camera* cam,
    Vec3 center, Vec3 axis, float radius,
    int screenW, int screenH, ImU32 color, float thickness)
{
    // Build two orthogonal vectors perpendicular to axis
    Vec3 u, v;
    if (std::abs(axis.x) < 0.9f) {
        u = normalize(cross(axis, Vec3{ 1, 0, 0 }));
    }
    else {
        u = normalize(cross(axis, Vec3{ 0, 1, 0 }));
    }
    v = normalize(cross(axis, u));

    const int segments = 48;
    ImVec2 lastPoint;
    bool hasLast = false;

    for (int i = 0; i <= segments; i++) {
        float angle = (i / (float)segments) * 2.0f * 3.14159265f;
        Vec3 point = center + u * (std::cosf(angle) * radius)
            + v * (std::sinf(angle) * radius);
        Vec3 cs = view * point;
        ScreenVert sv;
        if (cam->project(cs, screenW, screenH, sv)) {
            ImVec2 sp(sv.x, sv.y);
            if (hasLast) draw->AddLine(lastPoint, sp, color, thickness);
            lastPoint = sp;
            hasLast = true;
        }
        else {
            hasLast = false;
        }
    }
}

void Editor::drawGizmo(Scene& scene, int screenW, int screenH, Object* cameraObj) {
    if (selectedObjectIndex < 0 || selectedObjectIndex >= (int)scene.objects.size()) return;

    Object* obj = scene.objects[selectedObjectIndex].get();
    Camera* cam = cameraObj->getComponent<Camera>();
    if (!cam) return;

    Mat4 view = cam->viewMatrix(cameraObj->transform);

    // World-space axis endpoints
    const float axisLength = 1.0f;
    Vec3 origin = obj->transform.position;
    Vec3 xEndPos = origin + Vec3{ axisLength, 0, 0 };
    Vec3 yEndPos = origin + Vec3{ 0, axisLength, 0 };
    Vec3 zEndPos = origin + Vec3{ 0, 0, axisLength };

    // Project to screen
    auto projectPoint = [&](Vec3 worldPos, ImVec2& out) -> bool {
        Vec3 camSpace = view * worldPos;
        ScreenVert sv;
        if (!cam->project(camSpace, screenW, screenH, sv)) return false;
        out = ImVec2(sv.x, sv.y);
        return true;
	};

    ImVec2 sOrigin, sX, sY, sZ;
    if (!projectPoint(origin, sOrigin)) return;
    bool hasX = projectPoint(xEndPos, sX);
    bool hasY = projectPoint(yEndPos, sY);
    bool hasZ = projectPoint(zEndPos, sZ);

    ImDrawList* draw = ImGui::GetForegroundDrawList();
    const float thickness = 4.0f;
    auto axisColor = [&](GizmoAxis axis, ImU32 normal, ImU32 highlight) {
        return (activeGizmoAxis == axis || hoveredGizmoAxis == axis) ? highlight : normal;
	};

    ImU32 xColor = axisColor(GizmoAxis::X, IM_COL32(180, 40, 40, 255), IM_COL32(255, 100, 100, 255));
    ImU32 yColor = axisColor(GizmoAxis::Y, IM_COL32(40, 180, 40, 255), IM_COL32(100, 255, 100, 255));
    ImU32 zColor = axisColor(GizmoAxis::Z, IM_COL32(40, 40, 180, 255), IM_COL32(100, 100, 255, 255));

    if (gizmoMode == GizmoMode::Rotate) {
        const float radius = 1.0f;
        drawCircle(draw, view, cam, origin, Vec3{ 1,0,0 }, radius, screenW, screenH, xColor, thickness);
        drawCircle(draw, view, cam, origin, Vec3{ 0,1,0 }, radius, screenW, screenH, yColor, thickness);
        drawCircle(draw, view, cam, origin, Vec3{ 0,0,1 }, radius, screenW, screenH, zColor, thickness);
    }
    else {
        if (hasX) draw->AddLine(sOrigin, sX, xColor, thickness);
        if (hasY) draw->AddLine(sOrigin, sY, yColor, thickness);
        if (hasZ) draw->AddLine(sOrigin, sZ, zColor, thickness);
    }
}

void Editor::drawSceneHierarchy(Scene& scene) {
    ImGui::Begin("Scene");
    if (ImGui::Button("Save")) scene.save("scene.json");
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        scene.load("scene.json");
        selectedObjectIndex = -1;  // selection invalid after reload
    }
    ImGui::SameLine();
    if (ImGui::Button("New Object")) {
        auto obj = std::make_unique<Object>();
        obj->transform.position = { 0, 0, 0 };
        scene.objects.push_back(std::move(obj));
        selectedObjectIndex = (int)scene.objects.size() - 1;
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete Object")) {
        if (selectedObjectIndex >= 0 && selectedObjectIndex < (int)scene.objects.size()) {
            scene.objects.erase(scene.objects.begin() + selectedObjectIndex);
			if (selectedObjectIndex >= (int)scene.objects.size()) {
                selectedObjectIndex = (int)scene.objects.size() - 1;
            }
        }
    }
    ImGui::Separator();
    for (int i = 0; i < (int)scene.objects.size(); i++) {
        char label[64];
        snprintf(label, sizeof(label), "Object %d", i);
        if (ImGui::Selectable(label, selectedObjectIndex == i)) {
            selectedObjectIndex = i;
        }
    }
    ImGui::Separator();
    ImGui::Text("Mode: %s", gizmoMode == GizmoMode::Translate ? "Translate (W)" :
		gizmoMode == GizmoMode::Rotate ? "Rotate (R)" : "Scale (S)");
    ImGui::End();
}

void Editor::drawInspector(Scene& scene) {
    ImGui::Begin("Inspector");

    if (selectedObjectIndex < 0 || selectedObjectIndex >= (int)scene.objects.size()) {
        ImGui::Text("No object selected");
        ImGui::End();
        return;
    }

    Object* obj = scene.objects[selectedObjectIndex].get();

    // Transform
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat3("Position", &obj->transform.position.x, 0.1f);
        ImGui::DragFloat("Yaw", &obj->transform.yaw, 0.01f);
        ImGui::DragFloat("Pitch", &obj->transform.pitch, 0.01f);
        ImGui::DragFloat("Roll", &obj->transform.roll, 0.01f);
		ImGui::DragFloat3("Scale", &obj->transform.scale.x, 0.1f);
    }

    // Components
    for (size_t i = 0; i < obj->components.size(); i++) {
        auto& comp = obj->components[i];
        if (ImGui::CollapsingHeader(comp->typeName(), ImGuiTreeNodeFlags_DefaultOpen)) {
            comp->drawInspector();
            ImGui::PushID((int)i);
            if (ImGui::SmallButton("Remove")) {
                obj->components[i]->onDestroy();
                obj->components.erase(obj->components.begin() + i);
                ImGui::PopID();
                break;
            }
            ImGui::PopID();
        }
    }
    ImGui::Separator();
    if (ImGui::Button("Add Component")) {
        ImGui::OpenPopup("AddComponentPopup");
    }

    if (ImGui::BeginPopup("AddComponentPopup")) {
        const char* types[] = { "MeshRenderer", "BoxCollider", "SphereCollider", "Rigidbody", "AudioSource", "Camera"};
        for (const char* t : types) {
            if (ImGui::Selectable(t)) {
                auto comp = createComponent(t);
                if (comp) {
                    comp->owner = obj;
                    obj->components.push_back(std::move(comp));
                }
            }
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}