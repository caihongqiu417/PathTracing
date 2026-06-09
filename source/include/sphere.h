#pragma once

#include"ray.h"
#include<optional>

struct Sphere {
	glm::vec3 center;
	float radius;

	/// @brief 计算射线与球体的交代呢举例
	/// @param ray 相机到像素点的射线 ray.origin + t × ray.direction
	/// @return 如果有交点，返回从射线起点到交点的距离t；否则返回空
	std::optional<float> Intersect(const Ray& ray)const;
};