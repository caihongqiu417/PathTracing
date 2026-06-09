#include "sphere.h"

#include "hit_info.h"

#include <cmath>

std::optional<HitInfo> Sphere::Intersect(const Ray& ray, float t_min, float t_max) const
{
	glm::vec3 oc = ray.origin - center;

	float a = glm::dot(ray.direction, ray.direction);
	float b = 2.f * glm::dot(ray.direction, oc);
	float c = glm::dot(oc, oc) - radius * radius;
	float delta = b * b - 4.f *a*c;

	if (delta < 0.f)
		return {};

	float hit_t = (-b - std::sqrt(delta)) * 0.5f/a;

	if (hit_t < t_min || hit_t > t_max)
	{
		hit_t = (-b + std::sqrt(delta)) * 0.5f/a;
		if (hit_t < t_min || hit_t > t_max)
			return {};
	}

	glm::vec3 point = ray.Hit(hit_t);
	glm::vec3 normal = glm::normalize(point - center);
	return HitInfo{ hit_t, point, normal };
}
