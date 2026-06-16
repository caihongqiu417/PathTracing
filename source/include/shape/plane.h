#pragma once

#include "shape.h"

// 有限圆盘，由圆心、法线、半径定义
struct Plane : public Shape
{
	Plane(const glm::vec3& point, const glm::vec3& normal, float radius)
		: point(point), normal(glm::normalize(normal)), radius(radius)
	{
		// 在盘面内建立局部坐标系（x_axis, z_axis 互相垂直且均垂直于 normal）
		glm::vec3 up = glm::abs(normal.y) < 0.99999f ? glm::vec3(0, 1, 0) : glm::vec3(0, 0, -1);
		x_axis = glm::normalize(-glm::cross(up, normal));
		z_axis = glm::normalize(glm::cross(x_axis, normal));

		// 在局部坐标系中，圆盘沿 x_axis/z_axis 延伸 radius，沿 normal 厚度为 0.002
		Bounds bounds_local{ {-radius, -0.001f, -radius}, {radius, 0.001f, radius} };

		// 将局部包围盒的 8 个顶角变换到世界空间，取 min/max 得到世界 AABB
		// 变换公式：world = point + c.x*x_axis + c.y*normal + c.z*z_axis
		bounds = Bounds{ glm::vec3(FLT_MAX), glm::vec3(-FLT_MAX) };
		for (size_t i = 0; i < 8; i++)
		{
			glm::vec3 c = bounds_local.GetCorner(i);
			glm::vec3 world = point + c.x * x_axis + c.y * normal + c.z * z_axis;
			bounds.Expand(world);
		}
	}

	std::optional<HitInfo> Intersect(const Ray& ray, float t_min, float t_max) const override;

	float GetArea()const override;

	std::optional<ShapeSample> SampleShape(Rng& rng)const override;

	glm::vec3 point;            // 圆心（世界坐标）
	glm::vec3 normal;           // 法线（世界坐标，已归一化）
	glm::vec3 x_axis, z_axis;   // 盘面内的局部坐标轴
	float     radius;           // 圆盘半径
	Bounds    bounds;           // 世界空间 AABB
};
