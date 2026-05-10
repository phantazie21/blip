#pragma once
#include "Component.h"
#include "Mesh.h"
#include "../../math/Vec3.h"
#include <memory>

struct MeshRenderer : Component {
    std::unique_ptr<Mesh>  mesh;
    std::vector<Mesh>      lods;
    std::vector<Vec3>       worldVerts;
    std::vector<Vec3>       worldNormals;

	void update(float dt) override {}
	void fixedUpdate(float dt) override {}
};
