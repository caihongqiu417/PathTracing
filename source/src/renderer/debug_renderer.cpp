#include "renderer/debug_renderer.h"
#include "util/debug_macro.h"
#include "util/rgb.h"
#include "thread/spin_lock.h"
#include <iostream>

template<typename GetValue>
static glm::vec3 RenderHeatmapPixel(
    const Camera& camera, const Scene& scene, const glm::ivec2& pixel_coord,
    Rng& rng, float max_value, GetValue get_value)
{
    Ray ray = camera.GenerateRay(pixel_coord, rng.PixelOffset());
    auto hit_info = scene.Intersect(ray);
    DEBUG_LINE(
        if (hit_info.has_value() && max_value > 0)
            return RGB::GenerateHeatmapRGB(get_value(*hit_info) / max_value);
    );
    return glm::vec3(0.0f);
}

// 单线程扫全图找最大值，只调用一次
template<typename GetValue>
static float FindMaxValue(
    const Camera& camera, const Scene& scene, Rng& rng, GetValue get_value)
{
    float max_val = 1.f;
    DEBUG_LINE(
        int w = (int)camera.GetFilm().GetWidth();
        int h = (int)camera.GetFilm().GetHeight();
        for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
        {
            Ray ray = camera.GenerateRay({x, y}, rng.PixelOffset());
            auto hit = scene.Intersect(ray);
            if (hit.has_value())
                max_val = glm::max(max_val, (float)get_value(*hit));
        }
    );
    return max_val;
}

// ===========================================================================================
// BoundsTest
// ===========================================================================================

glm::vec3 BoundsTestRenderer::RenderPixel(const glm::ivec2& pixel_coord)
{
    DEBUG_LINE(
        // 双重检查锁：保证多线程下预扫描只跑一次
        if (!max_value_cached)
        {
            Guard guard(scan_lock);
            if (!max_value_cached)
            {
                max_bounds = FindMaxValue(camera, scene, rng,
                    [](const HitInfo& h){ return (float)h.bounds_test_count; });
                max_value_cached = true;
                std::cout << "[BoundsTest] 最大包围盒测试次数: " << (int)max_bounds << "\n";
            }
        }
        return RenderHeatmapPixel(camera, scene, pixel_coord, rng, max_bounds,
            [](const HitInfo& h){ return (float)h.bounds_test_count; });
    );
    return glm::vec3(0.0f);
}

// ===========================================================================================
// TriangleTest
// ===========================================================================================

glm::vec3 TriangleTestRenderer::RenderPixel(const glm::ivec2& pixel_coord)
{
    DEBUG_LINE(
        if (!max_value_cached)
        {
            Guard guard(scan_lock);
            if (!max_value_cached)
            {
                max_triangles = FindMaxValue(camera, scene, rng,
                    [](const HitInfo& h){ return (float)h.triangle_test_count; });
                max_value_cached = true;
                std::cout << "[TriangleTest] 最大三角形测试次数: " << (int)max_triangles << "\n";
            }
        }
        return RenderHeatmapPixel(camera, scene, pixel_coord, rng, max_triangles,
            [](const HitInfo& h){ return (float)h.triangle_test_count; });
    );
    return glm::vec3(0.0f);
}

// ===========================================================================================
// BoundsDepth
// ===========================================================================================

glm::vec3 BoundsDepthRenderer::RenderPixel(const glm::ivec2& pixel_coord)
{
    DEBUG_LINE(
        if (!max_value_cached)
        {
            Guard guard(scan_lock);
            if (!max_value_cached)
            {
                max_depth = FindMaxValue(camera, scene, rng,
                    [](const HitInfo& h){ return (float)h.bounds_depth; });
                max_value_cached = true;
                std::cout << "[BoundsDepth] 最大BVH深度: " << (int)max_depth << "\n";
            }
        }
        return RenderHeatmapPixel(camera, scene, pixel_coord, rng, max_depth,
            [](const HitInfo& h){ return (float)h.bounds_depth; });
    );
    return glm::vec3(0.0f);
}
