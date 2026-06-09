#pragma once

#include <glm/glm.hpp>

// 射线打中物体后返回的信息
struct HitInfo
{
	float t;              // 交点离射线起点有多远
	glm::vec3 point;      // 交点，使用世界空间
	glm::vec3 normal;     // 交点处的法线，使用世界空间
};
