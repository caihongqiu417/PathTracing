#pragma once

#include"shape.h"

struct Triangle : public Shape {
	// 带顶点法线的构造函数
	Triangle(
		const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2,
		const glm::vec3& n0, const glm::vec3& n1, const glm::vec3& n2
	) : p0(p0), p1(p1), p2(p2), n0(n0), n1(n1), n2(n2) {
	}

	// 不带法线的构造函数（自动计算平面法线）
	Triangle(
		const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2
	) : p0(p0), p1(p1), p2(p2) {
		glm::vec3 e1 = p1 - p0;
		glm::vec3 e2 = p2 - p0;
		glm::vec3 normal = glm::normalize(glm::cross(e1, e2));
		n0 = normal;
		n1 = normal;
		n2 = normal;
	}


	//相交测试
	std::optional<HitInfo> Intersect(const Ray& ray, float t_min, float t_max)const override;
	//顶点
	glm::vec3 p0, p1, p2;

	//法线
	glm::vec3 n0, n1, n2;
};