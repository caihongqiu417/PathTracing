#include "sphere.h"

#include "hit_info.h"

#include <cmath>

// 射线和球求交
std::optional<HitInfo> Sphere::Intersect(const Ray& ray, float t_min, float t_max) const
{
	glm::vec3 oc = ray.origin - center;

	// 一元二次方程
	float a = glm::dot(ray.direction, ray.direction);
	float b = 2.f * glm::dot(ray.direction, oc);
	float c = glm::dot(oc, oc) - radius * radius;
	float delta = b * b - 4.f *a*c;

	if (delta < 0.f)
		return {};

	// 先试较近的交点
	float hit_t = (-b - std::sqrt(delta)) * 0.5f/a;

	if (hit_t < t_min || hit_t > t_max)
	{
		// 再试较远的交点
		hit_t = (-b + std::sqrt(delta)) * 0.5f/a;
		if (hit_t < t_min || hit_t > t_max)
			return {};
	}

	// 交点和法线
	glm::vec3 point = ray.Hit(hit_t);
	glm::vec3 normal = glm::normalize(point - center);
	return HitInfo{ hit_t, point, normal };
}
