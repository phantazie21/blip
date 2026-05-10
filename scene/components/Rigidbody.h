#pragma once
#include "Component.h"
#include "../../math/Vec3.h"

struct Rigidbody : Component {
    Vec3  velocity = { 0, 0, 0 };
    float mass = 1.0f;
    bool  isStatic = false;  // true = immovable (floor, walls)
    bool  useGravity = true;

    void update(float dt) override {}
    void fixedUpdate(float dt) override {}
    const char* typeName() const override { return "Rigidbody"; }
    void drawInspector() override {
        ImGui::DragFloat3("Velocity", &velocity.x, 0.1f);
        ImGui::DragFloat("Mass", &mass, 0.1f, 0.001f, 1000.0f);
        ImGui::Checkbox("Use Gravity", &useGravity);
        ImGui::Checkbox("Is Static", &isStatic);
    }
    json toJson() const override {
        return {
            {"velocity",   {velocity.x, velocity.y, velocity.z}},
            {"mass",       mass},
            {"isStatic",   isStatic},
            {"useGravity", useGravity}
        };
    }
    void fromJson(const json& j) override {
        auto v = j["velocity"];
        velocity = { v[0], v[1], v[2] };
        mass = j.value("mass", 1.0f);
        isStatic = j.value("isStatic", false);
        useGravity = j.value("useGravity", true);
    }
};
