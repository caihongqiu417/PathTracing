#pragma once

#include <glm/glm.hpp>

// 射线，默认用世界空间表示
struct Ray
{
	glm::vec3 origin;
	glm::vec3 direction;

	glm::vec3 Hit(float t) const { return origin + t * direction; }

	// 把世界空间里的射线变到模型空间里
	Ray WorldToObject(const glm::mat4& object_from_world) const
	{
		// 起点会受平移影响
		glm::vec3 o = glm::vec3(object_from_world * glm::vec4(origin, 1.f));
		// 方向不受平移影响
		glm::vec3 d = glm::vec3(object_from_world * glm::vec4(direction, 0.f));
		return Ray{ o, glm::normalize(d) };
	}
};
