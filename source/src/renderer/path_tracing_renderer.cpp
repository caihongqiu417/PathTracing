#include "renderer/path_tracing_renderer.h"

#include "util/frame.h"
#include "sample/sphererical.h"
#include <glm/gtc/constants.hpp>

glm::vec3 PathTracingRenderer::RenderPixel(const glm::ivec2& pixel_coord)
{
	// -----------------------------------------------------------------------
	// 初始化
	// -----------------------------------------------------------------------
	auto ray = camera.GenerateRay(pixel_coord, rng.PixelOffset());

	glm::vec3 beta = { 1, 1, 1 };  // 当前路径的累积权重（每次弹射后衰减）
	glm::vec3 L    = { 0, 0, 0 };  // 累积 radiance

	constexpr float q  = 0.9f;              // 俄罗斯轮盘赌：继续的概率
	constexpr float pi = glm::pi<float>();

	// -----------------------------------------------------------------------
	// 路径追踪主循环：每轮弹射一次
	// -----------------------------------------------------------------------
	while (true)
	{
		auto hit_info = scene.Intersect(ray);

		// -- a. 无交点：结束路径 --
		if (!hit_info.has_value())
			break;

		// -- b. 俄罗斯轮盘赌：以 (1-q) 的概率提前终止，节省计算 --
		if (rng.Float01() > q)
			break;

		// -- c. 累积自发光贡献 --
		L += beta * hit_info->material->emissive;

		// -- d. 轮盘赌补偿：活下来的路径放大权重，保证期望值不变 --
		beta /= q;

		// -- e. 计算反射方向，并更新路径权重 --
		Frame frame(hit_info->normal);  // 以交点法线为 y 轴建立局部坐标系
		glm::vec3 new_direction;

		if (hit_info->material->is_specular)
		{
			// 镜面反射：方向唯一确定，直接乘反射率，不需要 brdf/pdf 公式
			glm::vec3 view_local = frame.LocalFromWorld(-ray.direction);
			new_direction = { -view_local.x, view_local.y, -view_local.z };
			beta *= hit_info->material->albedo;
		}
		else
		{
			// 漫反射：余弦重要性采样
			// pdf = cos(θ) / π，brdf = albedo / π
			// beta *= brdf * cos / pdf = (albedo/π) * cos / (cos/π) = albedo，cos 约掉了
			new_direction = CosineSampleHemisphere({ rng.Float01(), rng.Float01() });
			beta *= hit_info->material->albedo;
		}

		// -- f. 更新光线，准备下一次弹射 --
		ray.origin    = hit_info->point + hit_info->normal * 1e-4f;  // 沿法线偏移，避免自相交
		ray.direction = frame.WorldFromLocal(new_direction);
	}

	return L;
}
