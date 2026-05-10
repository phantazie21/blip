#pragma once

#include "Object.h"
#include "../renderer/Camera.h"
#include <memory>
#include <vector>
#include "components/MeshRenderer.h"
#include "CollisionSystem.h"

struct Scene {
	CollisionSystem collisionSystem;
    std::vector<ContactInfo> contacts;
    std::vector<std::unique_ptr<Object>> objects;
    Vec3  lightDir = normalize({ 0.5f, 1.0f, 0.0f });
    float ambientLight = 0.15f;
    void render(Renderer& r, const Camera& cam);
    void processFace(Mesh* mesh, const MeshRenderer* mr, const Face& f, Renderer& r, const Camera& cam, const Mat4& mv);
	void update(float dt);
    void fixedUpdate(float dt);
};
