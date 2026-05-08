#pragma once
#include <cstdint>
#include "../math/Vec2.h"

struct ScreenVert {
    float x, y, z;
    bool visible;
};

struct Texture {
    int width, height;
	uint32_t* data = nullptr;

    ~Texture() { delete[] data; }

    Texture() = default;
	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;
};

inline uint32_t applyIntensity(uint32_t color, float intensity) {
    uint32_t r = (color & 0xff) * intensity;
    uint32_t g = ((color >> 8) & 0xff) * intensity;
    uint32_t b = ((color >> 16) & 0xff) * intensity;
    return (255u << 24) | (b << 16) | (g << 8) | r;
}

struct DrawCall {
    ScreenVert a, b, c;
    Vec2 uv0, uv1, uv2;
    const Texture* texture;
    uint32_t color;
    float i0, i1, i2;
};

struct Renderer;

void drawTriangle(Renderer& r, ScreenVert a, ScreenVert b, ScreenVert c, Vec2 uv0, Vec2 uv1, Vec2 uv2, const Texture* texture, uint32_t color, float i0, float i1, float i2, int clipYMin, int clipYMax);
