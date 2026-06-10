#include "renderer/rt_renderer.h"

#include "util/frame.h"

glm::vec3 RTRenderer::RenderPixel(const glm::ivec2& pixel_coord)
{
	constexpr int max_bounces = 8;
	constexpr float ray_origin_bias = 1e-4f;

	// 当前样本的颜色
	glm::vec3 beta = { 1.f, 1.f, 1.f };
	glm::vec3 color = { 0.f, 0.f, 0.f };

	// 主光线。抖动
	Ray ray = camera.GenerateRay(pixel_coord, rng.PixelOffset());

	// 路径反弹
	for (int bounce = 0; bounce < max_bounces; ++bounce)
	{
		auto hit_info = scene.Intersect(ray);
		if (!hit_info.has_value())
			break;

		// 累加这一跳的发光。
		color += beta * hit_info->material->emissive;
		beta *= hit_info->material->albedo;

		// 新光线从表面上方一点点出发。
		ray.origin = hit_info->point + hit_info->normal * ray_origin_bias;

		Frame frame(hit_info->normal);
		glm::vec3 light_direction;
		if (hit_info->material->is_specular)
		{
			// 镜面反射
			glm::vec3 view_direction = frame.LocalFromWorld(-ray.direction);
			light_direction = { -view_direction.x, view_direction.y, -view_direction.z };
		}
		else
		{
			// 漫反射
			light_direction = rng.SampleHemisphere();
		}

		// 转回世界空间
		ray.direction = frame.WorldFromLocal(light_direction);
	}

	return color;
}
