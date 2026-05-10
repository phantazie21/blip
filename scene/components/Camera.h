#pragma once
#include "Component.h"
#include "../math/Vec3.h"
#include "../math/Mat4.h"
#include "../../renderer/Renderer.h"
#include "../Transform.h"

struct Camera : Component {
    float fov = 3.14159f / 3.0f;
    float focal = 1.0f / std::tanf(3.14159f / 6.0f);
    float nearZ = 0.01f;
    float farZ = 1000.0f;
    bool  isMain = true;  // which camera renders the scene

    void update(float dt) override {}
    void fixedUpdate(float dt) override {}
    const char* typeName() const override { return "Camera"; }
    void drawInspector() override;

    // moved from old Camera:
    Mat4 viewMatrix(const Transform& t) const;  // takes object transform
    bool project(Vec3 v, int screenW, int screenH, ScreenVert& out) const;
    void setFov(float f);

    json toJson() const override;
    void fromJson(const json& j) override;
};
