#pragma once
#include <string>
#include <memory>
#include "../scene/Mesh.h"

std::unique_ptr<Mesh> loadOBJ(const std::string& path, uint32_t color, float scale = 1.0f);