#include "renderer/debug_renderer.h"
#include "util/debug_macro.h"
#include "util/rgb.h"
#include <iostream>

// ===========================================================================================
// BoundsTest 渲染器：显示包围盒检测次数
// ===========================================================================================

glm::vec3 BoundsTestRenderer::RenderPixel(const glm::ivec2& pixel_coord)
{
    Ray ray = camera.GenerateRay(pixel_coord, rng.PixelOffset());
    auto hit_info = scene.Intersect(ray);

    DEBUG_LINE(
        if (hit_info.has_value()) {
            return RGB::GenerateHeatmapRGB(hit_info->bounds_test_count / 30.f);
        }
    );

    return glm::vec3(0.0f);
}

// ===========================================================================================
// TriangleTest 渲染器：显示三角形检测次数
// ===========================================================================================

glm::vec3 TriangleTestRenderer::RenderPixel(const glm::ivec2& pixel_coord)
{
    Ray ray = camera.GenerateRay(pixel_coord, rng.PixelOffset());
    auto hit_info = scene.Intersect(ray);

    DEBUG_LINE(
        if (hit_info.has_value()) {
            return RGB::GenerateHeatmapRGB(hit_info->triangle_test_count / 20000.f);
        }
    );

    return glm::vec3(0.0f);
}

// ===========================================================================================
// BoundsDepth 渲染器：显示BVH树遍历深度
// ===========================================================================================

glm::vec3 BoundsDepthRenderer::RenderPixel(const glm::ivec2& pixel_coord)
{
    Ray ray = camera.GenerateRay(pixel_coord, rng.PixelOffset());
    auto hit_info = scene.Intersect(ray);

    DEBUG_LINE(
        if (hit_info.has_value()) {
            return RGB::GenerateHeatmapRGB(hit_info->bounds_depth / 15.f);
        }
    );

    return glm::vec3(0.0f);
}
