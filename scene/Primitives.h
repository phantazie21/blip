#pragma once
#include "components/Mesh.h"
#include "components/Rigidbody.h"
#include <cmath>

inline std::unique_ptr<Mesh> createBox(float halfSize, uint32_t color) {
    auto mesh = std::make_unique<Mesh>();

    float h = halfSize;
    mesh->vertices = {
        { h,  h,  h}, {-h,  h,  h}, {-h, -h,  h}, { h, -h,  h}, // front
        { h,  h, -h}, {-h,  h, -h}, {-h, -h, -h}, { h, -h, -h}, // back
    };

    mesh->vertexUVs = {
        {0, 0}, {1, 0}, {1, 1}, {0, 1}
    };

    auto face = [&](int a, int b, int c, int ua, int ub, int uc) {
        Face f{};
        f.idx[0] = a; f.idx[1] = b; f.idx[2] = c;
        f.uv[0] = ua; f.uv[1] = ub; f.uv[2] = uc;
        f.normal[0] = f.normal[1] = f.normal[2] = -1; // computed below
        f.color = color;
        f.texture = nullptr;
        mesh->faces.push_back(f);
        };

    // front  (z+)
    face(0, 1, 2, 0, 1, 2);
    face(0, 2, 3, 0, 2, 3);
    // back   (z-)
    face(5, 4, 7, 0, 1, 2);
    face(5, 7, 6, 0, 2, 3);
    // top    (y+)
    face(1, 0, 4, 0, 1, 2);
    face(1, 4, 5, 0, 2, 3);
    // bottom (y-)
    face(3, 2, 6, 0, 1, 2);
    face(3, 6, 7, 0, 2, 3);
    // left   (x-)
    face(2, 1, 5, 0, 1, 2);
    face(2, 5, 6, 0, 2, 3);
    // right  (x+)
    face(0, 3, 7, 0, 1, 2);
    face(0, 7, 4, 0, 2, 3);

    computeVertexNormals(*mesh);
    mesh->bounds = computeBounds(mesh->vertices);
    return mesh;
}

inline std::unique_ptr<Object> createObjectWithMesh(std::unique_ptr<Mesh> mesh, Vec3 position, bool dynamic = false) {
    auto obj = std::make_unique<Object>();
	auto* mr = obj->addComponent<MeshRenderer>();
    mr->mesh = std::move(mesh);
    auto* bc = obj->addComponent<BoxCollider>();
    Vec3 minV = mr->mesh->vertices[0], maxV = mr->mesh->vertices[0];
    for (const auto& v : mr->mesh->vertices) {
        minV.x = std::min(minV.x, v.x); minV.y = std::min(minV.y, v.y); minV.z = std::min(minV.z, v.z);
        maxV.x = std::max(maxV.x, v.x); maxV.y = std::max(maxV.y, v.y); maxV.z = std::max(maxV.z, v.z);
    }
    bc->halfExtents = (maxV - minV) * 0.5f;	obj->transform.position = position;

    if (dynamic) {
        auto* rb = obj->addComponent<Rigidbody>();
        rb->isStatic = false;
	}
    return obj;
}
