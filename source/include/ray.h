#pragma once

#include<glm/glm.hpp>

struct Ray
{
	glm::vec3 origin;
	glm::vec3 direction;

	/// @brief 计算射线上的点
	/// @param t 距离（从射线起点开始）
	/// @return 射线上的点坐标
	glm::vec3 Hit(float t)const { return origin + t * direction; }
};