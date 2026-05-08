#include "Rasterizer.h"
#include "Renderer.h"

void drawTriangle(Renderer& r, ScreenVert a, ScreenVert b, ScreenVert c, Vec2 uv0, Vec2 uv1, Vec2 uv2, const Texture* texture, uint32_t color, float i0, float i1, float i2, int clipYMin, int clipYMax) {
    const int minX = std::max(0, (int)std::floor(std::min({ a.x, b.x, c.x })));
    const int maxX = std::min(r.width - 1, (int)std::ceil(std::max({ a.x, b.x, c.x })));
    const int minY = std::max(clipYMin, (int)std::floor(std::min({ a.y, b.y, c.y })));
    const int maxY = std::min(clipYMax, (int)std::ceil(std::max({ a.y, b.y, c.y })));
    const float inv_az = 1 / a.z;
    const float inv_bz = 1 / b.z;
    const float inv_cz = 1 / c.z;
    const float u0z = uv0.x * inv_az;
    const float v0z = uv0.y * inv_az;

    const float u1z = uv1.x * inv_bz;
    const float v1z = uv1.y * inv_bz;

    const float u2z = uv2.x * inv_cz;
    const float v2z = uv2.y * inv_cz;

    const float i0z = i0 * inv_az;
    const float i1z = i1 * inv_bz;
    const float i2z = i2 * inv_cz;

    const float cbx = c.x - b.x;
    const float cby = c.y - b.y;
    const float acx = a.x - c.x;
    const float acy = a.y - c.y;
    const float bax = b.x - a.x;
    const float bay = b.y - a.y;

    float w0_dx = -cby, w0_dy = cbx;
    float w1_dx = -acy, w1_dy = acx;
    float w2_dx = -bay, w2_dy = bax;

    float y0 = minY + 0.5f;
    float x0 = minX + 0.5f;

    float w0_row = (y0 - b.y) * cbx - (x0 - b.x) * cby;
    float w1_row = (y0 - c.y) * acx - (x0 - c.x) * acy;
    float w2_row = (y0 - a.y) * bax - (x0 - a.x) * bay;

    const float area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    if (std::abs(area) < 1e-8f) return;
    if (area < 0) {
        w0_dx = -w0_dx; w0_dy = -w0_dy;
        w1_dx = -w1_dx; w1_dy = -w1_dy;
        w2_dx = -w2_dx; w2_dy = -w2_dy;
        w0_row = -w0_row;
        w1_row = -w1_row;
        w2_row = -w2_row;
    }
    const float invArea = 1.f / std::abs(area);

    const float invZ_dx = (w0_dx * inv_az + w1_dx * inv_bz + w2_dx * inv_cz) * invArea;
    const float invZ_dy = (w0_dy * inv_az + w1_dy * inv_bz + w2_dy * inv_cz) * invArea;
    float invZ_row = (w0_row * inv_az + w1_row * inv_bz + w2_row * inv_cz) * invArea;
    const float uoz_dx = (w0_dx * u0z + w1_dx * u1z + w2_dx * u2z) * invArea;
    const float uoz_dy = (w0_dy * u0z + w1_dy * u1z + w2_dy * u2z) * invArea;

    const float voz_dx = (w0_dx * v0z + w1_dx * v1z + w2_dx * v2z) * invArea;
    const float voz_dy = (w0_dy * v0z + w1_dy * v1z + w2_dy * v2z) * invArea;

    float uoz_row = (w0_row * u0z + w1_row * u1z + w2_row * u2z) * invArea;
    float voz_row = (w0_row * v0z + w1_row * v1z + w2_row * v2z) * invArea;

    const float ioz_dx = (w0_dx * i0z + w1_dx * i1z + w2_dx * i2z) * invArea;
    const float ioz_dy = (w0_dy * i0z + w1_dy * i1z + w2_dy * i2z) * invArea;
    float ioz_row = (w0_row * i0z + w1_row * i1z + w2_row * i2z) * invArea;

    int rowStart = minY * r.width;
    const int row_dy = r.width;

    int texW, texH;
    uint32_t* texData = nullptr;
    if (texture) {
        texW = texture->width;
        texH = texture->height;
        texData = texture->data;
    }
    uint32_t* framebuffer = r.framebuffer;
    for (int y = minY; y <= maxY; y++) {
        float w0 = w0_row, w1 = w1_row, w2 = w2_row;
        int idx = rowStart + minX;
        float invZ = invZ_row;
        float uoz = uoz_row;
        float voz = voz_row;
        float ioz = ioz_row;
        bool entered = false;
        for (int x = minX; x <= maxX; x++) {
            if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f) {
                entered = true;
                if (invZ > r.depthBuffer[idx]) {
                    r.depthBuffer[idx] = invZ;
                    const float z = 1 / invZ;
                    const float intensity = ioz * z;
                    uint32_t finalColor = color;
                    if (texture) {
                        const float u = uoz * z;
                        const float v = voz * z;
                        const float uClamped = u < 0 ? 0 : (u > 1 ? 1 : u);
                        const float vClamped = v < 0 ? 0 : (v > 1 ? 1 : v);

                        const int tx = (uClamped * (texW - 1));
                        const int ty = (vClamped * (texH - 1));

                        finalColor = applyIntensity(texData[ty * texW + tx], intensity);
                    }
                    else {
                        finalColor = applyIntensity(color, intensity);
                    }
                    framebuffer[idx] = finalColor;
                }
            }
            else if (entered) {
                break;
            }
            w0 += w0_dx; w1 += w1_dx; w2 += w2_dx;
            uoz += uoz_dx;
            voz += voz_dx;
            idx++;
            invZ += invZ_dx;
            ioz += ioz_dx;
        }
        w0_row += w0_dy; w1_row += w1_dy; w2_row += w2_dy;
        uoz_row += uoz_dy; voz_row += voz_dy;
        invZ_row += invZ_dy;
        rowStart += row_dy;
        ioz_row += ioz_dy;
    }
}
