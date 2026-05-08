#pragma once

#include "Object.h"
#include "../renderer/Camera.h"
#include <memory>
#include <vector>

struct Scene {
    std::vector<std::unique_ptr<Object>> objects;
    Vec3  lightDir = normalize({ 0.5f, 1.0f, 0.0f });
    float ambientLight = 0.15f;
    void render(Renderer& r, const Camera& cam);
};
