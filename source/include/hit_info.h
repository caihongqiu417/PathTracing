#pragma once

#include <glm/glm.hpp>


/// @brief ShadingPoint信息
struct HitInfo
{
	float t;              // 沿射线的参数距离
	glm::vec3 point;      // 交点世界坐标
	glm::vec3 normal;     // 交点处 outward 法线
};
