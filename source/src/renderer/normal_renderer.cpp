#include "renderer/normal_renderer.h"

glm::vec3 NormalRenderer::RenderPixel(const glm::ivec2& pixel_coord)
{
	// 主光线。抖动
	auto ray = camera.GenerateRay(pixel_coord, rng.PixelOffset());
	auto hit_info = scene.Intersect(ray);
	if (hit_info.has_value())
		// 法线转到 [0, 1]
		return hit_info->normal * 0.5f + 0.5f;

	return { 0.f, 0.f, 0.f };
}