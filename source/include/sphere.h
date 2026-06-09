#pragma once

#include "shape.h"

// =============================================================================
// Sphere — 球体
// =============================================================================
class Sphere : public Shape
{
public:
	Sphere(const glm::vec3& center, float radius)
		: center(center), radius(radius)
	{
	}

	glm::vec3 center;
	float radius;

	std::optional<HitInfo> Intersect(
		const Ray& ray,
		float t_min = 1e-5f,
		float t_max = std::numeric_limits<float>::infinity()
	) const override;
};
