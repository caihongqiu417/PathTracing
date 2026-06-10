#pragma once

#include"shape.h"

//=========================================================
//	平面类：用面上一个点和一个法向量定义
//=========================================================
struct Plane :public Shape
{
	//构造
	Plane(const glm::vec3& point, const glm::vec3& normal)
		:point(point), normal(glm::normalize(normal)) {
	}

	//相交测试
	std::optional<HitInfo> Intersect(const Ray& ray, float t_min, float t_max)const override;

	glm::vec3 point;
	glm::vec3 normal;
};
