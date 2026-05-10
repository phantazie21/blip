#pragma once
#include <memory>
#include "components/Mesh.h"
#include "../math/Vec3.h"

struct Object;  // forward declaration is fine for the return type

std::unique_ptr<Mesh> createBox(float halfSize, uint32_t color);
std::unique_ptr<Object> createObjectWithMesh(std::unique_ptr<Mesh> mesh, Vec3 position, bool dynamic = false);