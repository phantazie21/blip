#include "Editor.h"
#include "imgui.h"
#include "../scene/Scene.h"
#include "../scene/Object.h"

void Editor::draw(Scene& scene) {
    drawSceneHierarchy(scene);
    drawInspector(scene);
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
    }

    // Components
    for (size_t i = 0; i < obj->components.size(); i++) {
        auto& comp = obj->components[i];
        if (ImGui::CollapsingHeader(comp->typeName(), ImGuiTreeNodeFlags_DefaultOpen)) {
            comp->drawInspector();
            ImGui::PushID((int)i);
            if (ImGui::SmallButton("Remove")) {
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
        const char* types[] = { "MeshRenderer", "BoxCollider", "SphereCollider", "Rigidbody" };
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