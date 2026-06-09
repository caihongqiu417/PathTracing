#include "plane.h"

#include <cmath>

std::optional<HitInfo> Plane::Intersect(const Ray& ray, float t_min, float t_max) const
{
	const float denom = glm::dot(ray.direction, normal);
	if (std::fabs(denom) < 1e-8f)
		return {};

	float hit_t = glm::dot(point - ray.origin, normal) / denom;
	if (hit_t > t_min && hit_t < t_max)
		return HitInfo{ hit_t, ray.Hit(hit_t), normal };

	return {};
}
