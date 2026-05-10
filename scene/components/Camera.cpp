#include "Camera.h"
#include "imgui.h"
#include <cmath>

Mat4 Camera::viewMatrix(const Transform& t) const {
    return rotateYZ(t.pitch) * rotateXZ(-t.yaw) * translation(-t.position);
}

bool Camera::project(Vec3 camSpaceVert, int screenW, int screenH, ScreenVert& out) const {
    if (camSpaceVert.z <= nearZ) return false;
    const float invZ = 1.f / camSpaceVert.z;
    const float aspect = (float)screenW / screenH;
    const float pX = (camSpaceVert.x * invZ) * focal / aspect;
    const float pY = (camSpaceVert.y * invZ) * focal;
    out.x = (pX + 1) * 0.5f * screenW;
    out.y = (1 - pY) * 0.5f * screenH;
    out.z = camSpaceVert.z;
    return true;
}

void Camera::setFov(float f) {
    fov = f;
    focal = 1.0f / std::tanf(fov * 0.5f);
}

void Camera::drawInspector() {
    float fovDeg = fov * 180.0f / 3.14159265f;
    if (ImGui::DragFloat("FOV", &fovDeg, 0.5f, 1.0f, 179.0f)) {
        setFov(fovDeg * 3.14159265f / 180.0f);
    }
    ImGui::DragFloat("Near Z", &nearZ, 0.001f, 0.001f, 10.0f);
    ImGui::DragFloat("Far Z", &farZ, 1.0f, 1.0f, 10000.0f);
    ImGui::Checkbox("Is Main", &isMain);
}

json Camera::toJson() const {
    return {
        {"fov", fov},
        {"nearZ", nearZ},
        {"farZ", farZ},
        {"isMain", isMain}
    };
}

void Camera::fromJson(const json& j) {
    setFov(j.value("fov", 3.14159f / 3.0f));  // rebuilds focal too
    nearZ = j.value("nearZ", 0.01f);
    farZ = j.value("farZ", 1000.0f);
    isMain = j.value("isMain", true);
}