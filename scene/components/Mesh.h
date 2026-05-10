#pragma once
#include <vector>
#include "../../math/Vec2.h"
#include "../../math/Vec3.h"
#include "../../renderer/Rasterizer.h"

struct Face {
    int idx[3];
    int uv[3];
    int normal[3];
    uint32_t color;
    const Texture* texture;
};

struct BoundingSphere {
    Vec3  center;
    float radius;
};

struct Mesh {
    std::vector<Vec3>     vertices;
    std::vector<Vec3>     vertexNormals;
    std::vector<Vec2>     vertexUVs;
    std::vector<Face>     faces;
    BoundingSphere        bounds;
};

BoundingSphere computeBounds(const std::vector<Vec3>& vertices);
void computeVertexNormals(Mesh& mesh);
