/* -*- C++ -*-
 *
 *  This file is part of RawTherapee.
 *
 *  RawTherapee is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RawTherapee is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RawTherapee.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "improcfun.h"
#include "labimage.h"
#include "procparams.h"
#include "rt_math.h"

namespace rtengine
{

namespace
{

// Uniform random value in [-1, 1] for an integer lattice point. A pure function of the coordinates, so
// the grain is identical in the preview, the detail windows and the saved image.
inline float latticeNoise(int32_t x, int32_t y, uint32_t seed)
{
    uint32_t h = static_cast<uint32_t>(x) * 0x8da6b343u ^ static_cast<uint32_t>(y) * 0xd8163841u ^ seed * 0xcb1ab31fu;
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return static_cast<float>(h & 0xffffffu) * (2.f / 16777215.f) - 1.f;
}

// Smooth value noise with one lattice cell per grain
inline float valueNoise(float x, float y, uint32_t seed)
{
    const float fx0 = std::floor(x);
    const float fy0 = std::floor(y);
    const int32_t x0 = static_cast<int32_t>(fx0);
    const int32_t y0 = static_cast<int32_t>(fy0);
    float tx = x - fx0;
    float ty = y - fy0;
    tx = tx * tx * (3.f - 2.f * tx);
    ty = ty * ty * (3.f - 2.f * ty);
    const float top = intp(tx, latticeNoise(x0 + 1, y0, seed), latticeNoise(x0, y0, seed));
    const float bottom = intp(tx, latticeNoise(x0 + 1, y0 + 1, seed), latticeNoise(x0, y0 + 1, seed));
    return intp(ty, bottom, top);
}

} // namespace

void ImProcFunctions::filmGrain(LabImage *lab, const procparams::FilmGrainParams &grainParams, int offsetX, int offsetY, double imageScale)
{
    if (!grainParams.enabled || grainParams.strength <= 0) {
        return;
    }

    const float size = std::max(0.3, grainParams.size);         // grain size in full-size pixels
    const float scale = std::max(1.0, imageScale);               // full-size pixels per pixel of lab
    // When one pixel of a downscaled view covers several grains they average out: the visible amplitude
    // drops roughly with the ratio of the sizes (keeps the preview close to the saved image)
    const float attenuation = scale > size ? size / scale : 1.f;
    const float amplitude = grainParams.strength / 100.f * 0.12f * 32768.f * attenuation;
    const float fine = 0.6f * grainParams.roughness / 100.f;     // weight of the finer second layer
    const float coarse = 1.f - fine;

#ifdef _OPENMP
    #pragma omp parallel for schedule(dynamic,16)
#endif

    for (int i = 0; i < lab->H; ++i) {
        const float y = (offsetY + i * scale) / size;

        for (int j = 0; j < lab->W; ++j) {
            const float x = (offsetX + j * scale) / size;
            const float noise = coarse * valueNoise(x, y, 1u) + fine * valueNoise(x * 2.3f + 17.f, y * 2.3f + 5.f, 2u);
            const float l = LIM01(lab->L[i][j] / 32768.f);
            // like film: strongest in the midtones, still present in shadows and highlights
            const float weight = 0.25f + 3.f * l * (1.f - l);
            lab->L[i][j] = LIM(lab->L[i][j] + amplitude * weight * noise, 0.f, 32768.f);
        }
    }
}

}
