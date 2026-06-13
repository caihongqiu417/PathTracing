#include "renderer/debug_renderer.h"
#include "util/debug_macro.h"
#include "util/rgb.h"
#include <iostream>

// 先扫一遍所有像素拿到最大值，再用最大值做分母归一化
// 避免写死分母导致颜色全压在紫色（太小）或全黄（太大）
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

template<typename GetValue>
static float FindMaxValue(
    const Camera& camera, const Scene& scene,
    Rng& rng, GetValue get_value)
{
    float max_val = 1.f;  // 至少为 1，防止除以 0
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
// BoundsTest 渲染器：显示包围盒检测次数（SceneBVH + 模型BVH 合计）
// ===========================================================================================

glm::vec3 BoundsTestRenderer::RenderPixel(const glm::ivec2& pixel_coord)
{
    DEBUG_LINE(
        if (!max_value_cached)
        {
            max_bounds = FindMaxValue(camera, scene, rng, [](const HitInfo& h){ return (float)h.bounds_test_count; });
            max_value_cached = true;
            std::cout << "[BoundsTest] 最大包围盒测试次数: " << (int)max_bounds << "\n";
        }
        return RenderHeatmapPixel(camera, scene, pixel_coord, rng, max_bounds,
            [](const HitInfo& h){ return (float)h.bounds_test_count; });
    );
    return glm::vec3(0.0f);
}

// ===========================================================================================
// TriangleTest 渲染器：显示三角形检测次数
// ===========================================================================================

glm::vec3 TriangleTestRenderer::RenderPixel(const glm::ivec2& pixel_coord)
{
    DEBUG_LINE(
        if (!max_value_cached)
        {
            max_triangles = FindMaxValue(camera, scene, rng, [](const HitInfo& h){ return (float)h.triangle_test_count; });
            max_value_cached = true;
            std::cout << "[TriangleTest] 最大三角形测试次数: " << (int)max_triangles << "\n";
        }
        return RenderHeatmapPixel(camera, scene, pixel_coord, rng, max_triangles,
            [](const HitInfo& h){ return (float)h.triangle_test_count; });
    );
    return glm::vec3(0.0f);
}

// ===========================================================================================
// BoundsDepth 渲染器：显示BVH树遍历深度
// ===========================================================================================

glm::vec3 BoundsDepthRenderer::RenderPixel(const glm::ivec2& pixel_coord)
{
    DEBUG_LINE(
        if (!max_value_cached)
        {
            max_depth = FindMaxValue(camera, scene, rng, [](const HitInfo& h){ return (float)h.bounds_depth; });
            max_value_cached = true;
            std::cout << "[BoundsDepth] 最大BVH深度: " << (int)max_depth << "\n";
        }
        return RenderHeatmapPixel(camera, scene, pixel_coord, rng, max_depth,
            [](const HitInfo& h){ return (float)h.bounds_depth; });
    );
    return glm::vec3(0.0f);
}
