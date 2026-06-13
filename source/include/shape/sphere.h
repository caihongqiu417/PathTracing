#pragma once

#include"ray.h"
#include"shape.h"
#include<optional>

struct Sphere :public Shape
{
	Sphere(const glm::vec3& center, float radius)
		:center(center), radius(radius) {
	}

	virtual std::optional<HitInfo> Intersect(const Ray& ray, float t_min, float t_max)const override;

	Bounds GetBounds()const { return { center - radius,center + radius }; }

	glm::vec3 center;
	float radius;
};