#pragma once
#include "Component.h"
#include "Mesh.h"
#include "../../math/Vec3.h"
#include <memory>
#include "../Primitives.h"

struct MeshRenderer : Component {
    std::unique_ptr<Mesh>  mesh;
    std::vector<Mesh>      lods;
    std::vector<Vec3>       worldVerts;
    std::vector<Vec3>       worldNormals;
    std::string meshType = "box";
    float meshParam = 0.5f;
    uint32_t color = 0xFFFFFFFF;

	void update(float dt) override {}
	void fixedUpdate(float dt) override {}
	const char* typeName() const override { return "MeshRenderer"; }
    void drawInspector() override {
        ImGui::Text("Mesh: %s", mesh ? "loaded" : "none");
        ImGui::Text("Faces: %d", mesh ? (int)mesh->faces.size() : 0);
        ImGui::Text("LODs: %d", (int)lods.size());
        bool changed = false;
        changed |= ImGui::DragFloat("Half Size", &meshParam, 0.01f, 0.01f, 10.0f);
        // color picker:
        ImVec4 col = ImGui::ColorConvertU32ToFloat4(color);
        if (ImGui::ColorEdit4("Color", &col.x)) {
            color = ImGui::ColorConvertFloat4ToU32(col);
            changed = true;
        }
        if (changed || !mesh) {
            mesh = createBox(meshParam, color);
        }
    }
    json toJson() const override {
        return { { "type", meshType }, { "param", meshParam }, { "color", color } };
	}
	void fromJson(const json& j) override {
        meshType = j.value("type", "box");
        meshParam = j.value("param", 0.5f);
        color = j.value("color", 0xFFFFFFFFu);
    }
};
