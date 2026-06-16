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

	// 获取自身表面积
	virtual float GetArea()const override;


	// 在几何体表面均匀采样一个点，用于直接光照（面光源采样）
	// 返回采样点的世界坐标、法线及对应 pdf；子类不实现则返回空（不可作面光源）
	virtual std::optional<ShapeSample> SampleShape(Rng& rng)const override;

	glm::vec3 center;
	float radius;
};