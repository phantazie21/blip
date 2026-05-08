#pragma once

#include "Mesh.h"
#include "../math/Mat4.h"
#include <memory>

struct Transform {
    Vec3  position = {};
    float yaw = 0, pitch = 0, roll = 0;
};

struct Object {
    std::unique_ptr<Mesh>  mesh;
    std::vector<Mesh>      lods;
    Transform           transform;

    std::vector<Vec3>       worldVerts;
    std::vector<Vec3>       worldNormals;
    std::vector<ScreenVert> projected;

    Mat4 worldMatrix() const {
		return translation(transform.position)
			* rotateXY(transform.roll)
			* rotateYZ(transform.pitch)
			* rotateXZ(transform.yaw);
    }
};
