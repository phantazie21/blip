#include "TextureLoader.h"
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

std::shared_ptr<Texture> loadTexture(const std::string& path) {
    int width, height, channels;

    unsigned char* raw = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (!raw) {
        std::cerr << "TextureLoader: failed to load: " << path << " (" << stbi_failure_reason() << ")\n";
        return nullptr;
    }

    auto tex = std::make_shared<Texture>();
    tex->width = width;
    tex->height = height;

    const size_t pixelCount = (size_t)width * height;
    tex->data = new uint32_t[pixelCount];
    std::memcpy(tex->data, raw, pixelCount * 4);

    stbi_image_free(raw);
    return tex;
}