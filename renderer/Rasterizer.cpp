#include "Rasterizer.h"
#include "Renderer.h"
#include <immintrin.h>

static inline void processRow(
    Renderer& r,
    int rowStart, int minX, int maxX,
    float w0_row, float w1_row, float w2_row,
    float w0_dx, float w1_dx, float w2_dx,
    float invZ_row, float invZ_dx,
    float uoz_row, float voz_row, float ioz_row,
    float uoz_dx, float voz_dx, float ioz_dx,
    const Texture* texture, uint32_t color,
    int texW, int texH, uint32_t* texData,
    uint32_t* framebuffer, uint32_t submitId)
{
    float w0 = w0_row, w1 = w1_row, w2 = w2_row;
    int   idx = rowStart + minX;
    float invZ = invZ_row;
    float uoz = uoz_row;
    float voz = voz_row;
    float ioz = ioz_row;
    bool  entered = false;

    for (int x = minX; x <= maxX; x++) {
        if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f) {
            entered = true;
			const float epsilon = std::abs(invZ) * 1e-3f;
			float dbValue = r.depthBuffer[idx];
            float diff = invZ - dbValue;
            bool depthGreater = diff > epsilon;
            bool depthEqual = std::abs(diff) <= epsilon;
            if (depthGreater || (depthEqual && submitId > r.idBuffer[idx])) {
                r.depthBuffer[idx] = invZ;
				r.idBuffer[idx] = submitId;
                const float z = 1.0f / invZ;
                const float intensity = ioz * z;
                uint32_t finalColor;
                if (texture) {
                    const float u = uoz * z;
                    const float v = voz * z;
                    const float uClamped = u < 0 ? 0 : (u > 1 ? 1 : u);
                    const float vClamped = v < 0 ? 0 : (v > 1 ? 1 : v);
                    const int tx = (int)(uClamped * (texW - 1));
                    const int ty = (int)(vClamped * (texH - 1));
                    finalColor = applyIntensity(texData[ty * texW + tx], intensity);
                }
                else {
                    finalColor = applyIntensity(color, intensity);
                }
                framebuffer[idx] = finalColor;
            }
        }
        else if (entered) break;

        w0 += w0_dx; w1 += w1_dx; w2 += w2_dx;
        idx++;
        invZ += invZ_dx;
        uoz += uoz_dx;
        voz += voz_dx;
        ioz += ioz_dx;
    }
}

static inline void processRow_AVX2(
    Renderer& r,
    int rowStart, int minX, int maxX,
    float w0_row, float w1_row, float w2_row,
    float w0_dx, float w1_dx, float w2_dx,
    float invZ_row, float invZ_dx,
    float uoz_row, float voz_row, float ioz_row,
    float uoz_dx, float voz_dx, float ioz_dx,
    const Texture* texture, uint32_t color,
    int texW, int texH, uint32_t* texData,
    uint32_t* framebuffer, uint32_t submitId)
{
    // Lane offsets for initialization: each of the 8 lanes starts at column x+lane
    const __m256 laneOffset = _mm256_set_ps(7, 6, 5, 4, 3, 2, 1, 0);

    // Per-pixel deltas, broadcast to all 8 lanes
    const __m256 v_w0_dx = _mm256_set1_ps(w0_dx);
    const __m256 v_w1_dx = _mm256_set1_ps(w1_dx);
    const __m256 v_w2_dx = _mm256_set1_ps(w2_dx);
    const __m256 v_invZ_dx = _mm256_set1_ps(invZ_dx);
    const __m256 v_uoz_dx = _mm256_set1_ps(uoz_dx);
    const __m256 v_voz_dx = _mm256_set1_ps(voz_dx);
    const __m256 v_ioz_dx = _mm256_set1_ps(ioz_dx);

    // Step values: how much each variable changes when we advance 8 pixels
    const __m256 v_eight = _mm256_set1_ps(8.0f);
    const __m256 v_w0_step = _mm256_mul_ps(v_w0_dx, v_eight);
    const __m256 v_w1_step = _mm256_mul_ps(v_w1_dx, v_eight);
    const __m256 v_w2_step = _mm256_mul_ps(v_w2_dx, v_eight);
    const __m256 v_invZ_step = _mm256_mul_ps(v_invZ_dx, v_eight);
    const __m256 v_uoz_step = _mm256_mul_ps(v_uoz_dx, v_eight);
    const __m256 v_voz_step = _mm256_mul_ps(v_voz_dx, v_eight);
    const __m256 v_ioz_step = _mm256_mul_ps(v_ioz_dx, v_eight);

    // Initial 8-lane values at the start of the scanline
    __m256 v_w0 = _mm256_add_ps(_mm256_set1_ps(w0_row), _mm256_mul_ps(laneOffset, v_w0_dx));
    __m256 v_w1 = _mm256_add_ps(_mm256_set1_ps(w1_row), _mm256_mul_ps(laneOffset, v_w1_dx));
    __m256 v_w2 = _mm256_add_ps(_mm256_set1_ps(w2_row), _mm256_mul_ps(laneOffset, v_w2_dx));
    __m256 v_invZ = _mm256_add_ps(_mm256_set1_ps(invZ_row), _mm256_mul_ps(laneOffset, v_invZ_dx));
    __m256 v_uoz = _mm256_add_ps(_mm256_set1_ps(uoz_row), _mm256_mul_ps(laneOffset, v_uoz_dx));
    __m256 v_voz = _mm256_add_ps(_mm256_set1_ps(voz_row), _mm256_mul_ps(laneOffset, v_voz_dx));
    __m256 v_ioz = _mm256_add_ps(_mm256_set1_ps(ioz_row), _mm256_mul_ps(laneOffset, v_ioz_dx));

    const __m256 v_zero = _mm256_setzero_ps();
    const __m256 v_one = _mm256_set1_ps(1.0f);

    // Pre-unpack the flat color once per scanline (only used in non-textured path)
    const __m256 v_color_r = _mm256_set1_ps((float)(color & 0xff));
    const __m256 v_color_g = _mm256_set1_ps((float)((color >> 8) & 0xff));
    const __m256 v_color_b = _mm256_set1_ps((float)((color >> 16) & 0xff));
    const __m256i v_alpha = _mm256_set1_epi32(0xFF000000);

    __m256i v_submitId = _mm256_set1_epi32((int)submitId);

    int x = minX;
    int idx = rowStart + minX;

    // ============================================================
    // Main loop: 8 pixels per iteration
    // ============================================================
    for (; x + 7 <= maxX; x += 8, idx += 8) {
        // Inside-triangle mask: w0 >= 0 && w1 >= 0 && w2 >= 0
        __m256 m_w0 = _mm256_cmp_ps(v_w0, v_zero, _CMP_GE_OQ);
        __m256 m_w1 = _mm256_cmp_ps(v_w1, v_zero, _CMP_GE_OQ);
        __m256 m_w2 = _mm256_cmp_ps(v_w2, v_zero, _CMP_GE_OQ);
        __m256 inside = _mm256_and_ps(m_w0, _mm256_and_ps(m_w1, m_w2));

        if (_mm256_movemask_ps(inside) == 0) {
            // No pixels inside this chunk — step and continue
            v_w0 = _mm256_add_ps(v_w0, v_w0_step);
            v_w1 = _mm256_add_ps(v_w1, v_w1_step);
            v_w2 = _mm256_add_ps(v_w2, v_w2_step);
            v_invZ = _mm256_add_ps(v_invZ, v_invZ_step);
            v_uoz = _mm256_add_ps(v_uoz, v_uoz_step);
            v_voz = _mm256_add_ps(v_voz, v_voz_step);
            v_ioz = _mm256_add_ps(v_ioz, v_ioz_step);
            continue;
        }

        // Depth test
        __m256 v_depth = _mm256_loadu_ps(&r.depthBuffer[idx]);
        __m256i v_id = _mm256_loadu_si256((__m256i*) & r.idBuffer[idx]);

        __m256 v_eps = _mm256_mul_ps(
            _mm256_andnot_ps(_mm256_set1_ps(-0.0f), v_invZ),  // |invZ|
            _mm256_set1_ps(1e-3f)
        );

        __m256 v_diff = _mm256_sub_ps(v_invZ, v_depth);
        __m256 v_absDiff = _mm256_andnot_ps(_mm256_set1_ps(-0.0f), v_diff); // abs

        __m256 depthGreater = _mm256_cmp_ps(v_diff, v_eps, _CMP_GT_OQ);
        __m256 depthEqual = _mm256_cmp_ps(v_absDiff, v_eps, _CMP_LE_OQ);
        __m256i idGreater_i = _mm256_cmpgt_epi32(v_submitId, v_id);
        __m256 idGreater = _mm256_castsi256_ps(idGreater_i);

        // pass: invZ > depth, OR (invZ == depth AND submitId > existingId)
        __m256 depthMask = _mm256_or_ps(depthGreater, _mm256_and_ps(depthEqual, idGreater));
        __m256 writeMask = _mm256_and_ps(inside, depthMask);
        int writeBits = _mm256_movemask_ps(writeMask);

        if (writeBits == 0) {
            v_w0 = _mm256_add_ps(v_w0, v_w0_step);
            v_w1 = _mm256_add_ps(v_w1, v_w1_step);
            v_w2 = _mm256_add_ps(v_w2, v_w2_step);
            v_invZ = _mm256_add_ps(v_invZ, v_invZ_step);
            v_uoz = _mm256_add_ps(v_uoz, v_uoz_step);
            v_voz = _mm256_add_ps(v_voz, v_voz_step);
            v_ioz = _mm256_add_ps(v_ioz, v_ioz_step);
            continue;
        }

        // Compute per-lane intensity = ioz * z = ioz * (1/invZ)
        __m256 v_z = _mm256_div_ps(v_one, v_invZ);
        __m256 v_intensity = _mm256_mul_ps(v_ioz, v_z);

        if (!texture) {
            // ====================================================
            // FAST PATH: flat color, fully vectorized
            // ====================================================
            __m256 nr = _mm256_mul_ps(v_color_r, v_intensity);
            __m256 ng = _mm256_mul_ps(v_color_g, v_intensity);
            __m256 nb = _mm256_mul_ps(v_color_b, v_intensity);

            // Truncate to int32 (matches scalar's `(int)` behavior)
            __m256i ir = _mm256_cvttps_epi32(nr);
            __m256i ig = _mm256_cvttps_epi32(ng);
            __m256i ib = _mm256_cvttps_epi32(nb);

            // Pack: (0xFF<<24) | (b<<16) | (g<<8) | r
            __m256i packed = _mm256_or_si256(
                v_alpha,
                _mm256_or_si256(
                    _mm256_or_si256(ir, _mm256_slli_epi32(ig, 8)),
                    _mm256_slli_epi32(ib, 16)
                )
            );

            // Masked stores: only write lanes where writeMask is set
            _mm256_maskstore_epi32((int*)&framebuffer[idx],
                _mm256_castps_si256(writeMask), packed);
            _mm256_maskstore_ps(&r.depthBuffer[idx],
                _mm256_castps_si256(writeMask), v_invZ);
            _mm256_maskstore_epi32((int*)&r.idBuffer[idx],
                _mm256_castps_si256(writeMask), v_submitId);
        }
        else {
            // ====================================================
            // SLOW PATH: textured, scalar lane loop (gather is slow)
            // ====================================================
            float intens[8], u_arr[8], v_arr[8];
            _mm256_storeu_ps(intens, v_intensity);
            _mm256_storeu_ps(u_arr, _mm256_mul_ps(v_uoz, v_z));
            _mm256_storeu_ps(v_arr, _mm256_mul_ps(v_voz, v_z));

            for (int lane = 0; lane < 8; lane++) {
                if (!(writeBits & (1 << lane))) continue;

                float u = u_arr[lane], v = v_arr[lane];
                float uc = u < 0 ? 0 : (u > 1 ? 1 : u);
                float vc = v < 0 ? 0 : (v > 1 ? 1 : v);
                int tx = (int)(uc * (texW - 1));
                int ty = (int)(vc * (texH - 1));
                framebuffer[idx + lane] = applyIntensity(texData[ty * texW + tx], intens[lane]);
            }

            _mm256_maskstore_ps(&r.depthBuffer[idx],
                _mm256_castps_si256(writeMask), v_invZ);
            _mm256_maskstore_epi32((int*)&r.idBuffer[idx],
                _mm256_castps_si256(writeMask), v_submitId);
        }

        // Step all per-pixel state forward by 8 pixels
        v_w0 = _mm256_add_ps(v_w0, v_w0_step);
        v_w1 = _mm256_add_ps(v_w1, v_w1_step);
        v_w2 = _mm256_add_ps(v_w2, v_w2_step);
        v_invZ = _mm256_add_ps(v_invZ, v_invZ_step);
        v_uoz = _mm256_add_ps(v_uoz, v_uoz_step);
        v_voz = _mm256_add_ps(v_voz, v_voz_step);
        v_ioz = _mm256_add_ps(v_ioz, v_ioz_step);
    }

    // ============================================================
    // Tail loop: handle remaining < 8 pixels with scalar code
    // ============================================================
    float w0_arr[8], w1_arr[8], w2_arr[8];
    float invZ_arr[8], uoz_arr[8], voz_arr[8], ioz_arr[8];
    _mm256_storeu_ps(w0_arr, v_w0);
    _mm256_storeu_ps(w1_arr, v_w1);
    _mm256_storeu_ps(w2_arr, v_w2);
    _mm256_storeu_ps(invZ_arr, v_invZ);
    _mm256_storeu_ps(uoz_arr, v_uoz);
    _mm256_storeu_ps(voz_arr, v_voz);
    _mm256_storeu_ps(ioz_arr, v_ioz);

    int lane = 0;
    for (; x <= maxX; x++, idx++, lane++) {
        if (w0_arr[lane] >= 0 && w1_arr[lane] >= 0 && w2_arr[lane] >= 0) {
			float diff = invZ_arr[lane] - r.depthBuffer[idx];
			float eps = std::abs(invZ_arr[lane]) * 1e-3f;
			bool pass = diff > eps || (std::abs(diff) <= eps && submitId > r.idBuffer[idx]);
            if (pass) {
                r.depthBuffer[idx] = invZ_arr[lane];
				r.idBuffer[idx] = submitId;
                float z = 1.0f / invZ_arr[lane];
                float intensity = ioz_arr[lane] * z;
                uint32_t finalColor;
                if (texture) {
                    float u = uoz_arr[lane] * z;
                    float v = voz_arr[lane] * z;
                    float uc = u < 0 ? 0 : (u > 1 ? 1 : u);
                    float vc = v < 0 ? 0 : (v > 1 ? 1 : v);
                    int tx = (int)(uc * (texW - 1));
                    int ty = (int)(vc * (texH - 1));
                    finalColor = applyIntensity(texData[ty * texW + tx], intensity);
                }
                else {
                    finalColor = applyIntensity(color, intensity);
                }
                framebuffer[idx] = finalColor;
            }
        }
    }
}
void drawTriangle(Renderer& r, ScreenVert a, ScreenVert b, ScreenVert c, Vec2 uv0, Vec2 uv1, Vec2 uv2, const Texture* texture, uint32_t color, float i0, float i1, float i2, int clipYMin, int clipYMax, uint32_t submitId) {
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

    int texW = 0, texH = 0;
    uint32_t* texData = nullptr;
    if (texture) {
        texW = texture->width;
        texH = texture->height;
        texData = texture->data;
    }
    uint32_t* framebuffer = r.framebuffer;
    for (int y = minY; y <= maxY; y++) {
        /*processRow(r, rowStart, minX, maxX,
            w0_row, w1_row, w2_row, w0_dx, w1_dx, w2_dx,
            invZ_row, invZ_dx,
            uoz_row, voz_row, ioz_row,
            uoz_dx, voz_dx, ioz_dx,
            texture, color, texW, texH, texData, framebuffer, submitId);*/

        processRow_AVX2(r, rowStart, minX, maxX,
            w0_row, w1_row, w2_row, w0_dx, w1_dx, w2_dx,
            invZ_row, invZ_dx,
            uoz_row, voz_row, ioz_row,
            uoz_dx, voz_dx, ioz_dx,
			texture, color, texW, texH, texData, framebuffer, submitId);

        w0_row += w0_dy;
        w1_row += w1_dy;
        w2_row += w2_dy;
        uoz_row += uoz_dy;
        voz_row += voz_dy;
        invZ_row += invZ_dy;
        ioz_row += ioz_dy;
        rowStart += row_dy;
    }
}
