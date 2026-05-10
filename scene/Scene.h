#pragma once

#include "Object.h"
#include "../renderer/Camera.h"
#include <memory>
#include <vector>
#include "components/MeshRenderer.h"
#include "CollisionSystem.h"
#include "AudioSystem.h"

struct Scene {
	CollisionSystem collisionSystem;
    std::vector<ContactInfo> contacts;
    std::vector<std::unique_ptr<Object>> objects;
    AudioSystem audioSystem;
    Vec3  lightDir = normalize({ 0.5f, 1.0f, 0.0f });
    float ambientLight = 0.15f;
    void render(Renderer& r, const Camera& cam);
    void processFace(Mesh* mesh, const MeshRenderer* mr, const Face& f, Renderer& r, const Camera& cam, const Mat4& mv);
	void update(float dt);
    void fixedUpdate(float dt);
	void save(const std::string& path) const;
	void load(const std::string& path);
	Scene();
};
