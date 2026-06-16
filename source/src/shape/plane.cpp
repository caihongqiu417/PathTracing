#include "plane.h"
#include"sample/sphererical.h"

#include <cmath>

std::optional<HitInfo> Plane::Intersect(const Ray& ray, float t_min, float t_max) const
{
	// dot(direction, normal) == 0 表示射线与平面平行，无交点
	const float denom = glm::dot(ray.direction, normal);
	if (std::fabs(denom) < 1e-8f)
		return {};

	// 平面方程 dot(P - point, normal) = 0，代入射线 P = origin + t*direction 解 t：
	//   t = dot(point - origin, normal) / dot(direction, normal)
	float hit_t = glm::dot(point - ray.origin, normal) / denom;
	if (hit_t <= t_min || hit_t >= t_max)
		return {};

	// 交点在无限平面上，还需判断是否落在圆盘半径内
	// 交点必然在盘面内（dot(offset, normal) == 0），offset 的长度即为距圆心的世界空间距离
	glm::vec3 hit_point = ray.Hit(hit_t);
	glm::vec3 offset    = hit_point - point;  // 世界空间：交点相对圆心的偏移
	if (glm::dot(offset, offset) > radius * radius)
		return {};

	return HitInfo{ hit_t, hit_point, normal };
}

float Plane::GetArea() const
{
	constexpr float PI = 3.141592653589793;
	return PI * radius * radius;
}

std::optional<ShapeSample> Plane::SampleShape(Rng& rng) const
{
	// 生成采样点（局部坐标系中）
	glm::vec2 sample_local = UniformSampleUnitDisk({ rng.Float01(), rng.Float01() });

	// 采样点转换到世界坐标
	glm::vec3 sample_point = point + sample_local.x * x_axis + sample_local.y * z_axis;

	return ShapeSample{ sample_point,normal,1.f / GetArea() };
}
