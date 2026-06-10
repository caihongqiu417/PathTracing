#pragma once

#include "shape.h"

// 平面
struct Plane : public Shape
{
	// 用点和法线定义
	Plane(const glm::vec3& point, const glm::vec3& normal)
		: point(point), normal(glm::normalize(normal))
	{
	}

	// 求交
	std::optional<HitInfo> Intersect(const Ray& ray, float t_min, float t_max) const override;

	glm::vec3 point;
	glm::vec3 normal;
};
