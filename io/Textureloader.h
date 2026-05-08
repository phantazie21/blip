#pragma once
#include <string>
#include <memory>
#include "../renderer/Rasterizer.h"

std::shared_ptr<Texture> loadTexture(const std::string& path);
