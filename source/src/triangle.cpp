#include "triangle.h"

#include <cmath>

std::optional<HitInfo> Triangle::Intersect(const Ray& ray, float t_min, float t_max) const
{
	// Möller–Trumbore 算法
	const glm::vec3 edge1 = p1 - p0;
	const glm::vec3 edge2 = p2 - p0;
	const glm::vec3 pvec = glm::cross(ray.direction, edge2);
	const float det = glm::dot(edge1, pvec);

	// 射线与三角形平面平行
	if (std::fabs(det) < 1e-8f)
		return {};

	const float inv_det = 1.f / det;
	const glm::vec3 tvec = ray.origin - p0;

	// 重心坐标 u
	const float u = glm::dot(tvec, pvec) * inv_det;
	if (u < 0.f || u > 1.f)
		return {};

	const glm::vec3 qvec = glm::cross(tvec, edge1);

	// 重心坐标 v
	const float v = glm::dot(ray.direction, qvec) * inv_det;
	if (v < 0.f || u + v > 1.f)
		return {};

	// 射线起点到交点的距离t
	const float t = glm::dot(edge2, qvec) * inv_det;
	if (t < t_min || t > t_max)
		return {};

	glm::vec3 point = (1 - u - v) * p0 + u * p1 + v * p2;
	glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

	// 法线朝向射线来源（与球体一致，便于 shading）
	if (glm::dot(normal, ray.direction) > 0.f)
		normal = -normal;

	return HitInfo{ t, point, normal };
}
