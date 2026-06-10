#pragma once

#include <cmath>

#include<glm/glm.hpp>

/// @brief 生成以法线向量为Y轴的局部空间，用于求解反射方向
class Frame
{
public:
	//根据法线（y轴）计算x轴，z轴
	Frame(const glm::vec3& normal)
	{
		y_axis = normal; // 法线为 Y 轴

		// 选一个不平行于法线的参考向量，避免叉乘出错
		const glm::vec3 up = std::abs(normal.y) < 0.99999f
			? glm::vec3(0.f, 1.f, 0.f)
			: glm::vec3(0.f, 0.f, 1.f);

		x_axis = glm::normalize(glm::cross(up, normal));
		z_axis = glm::normalize(glm::cross(x_axis, y_axis));
	}

	//世界空间转换到局部空间
	glm::vec3 LocalFromWorld(const glm::vec3& direction_world)const
	{
		return glm::normalize(glm::vec3{
			glm::dot(direction_world, x_axis),
			glm::dot(direction_world, y_axis),
			glm::dot(direction_world, z_axis),
		});
	}
	//局部空间转换到世界空间
	glm::vec3 WorldFromLocal(const glm::vec3& direction_local)const
	{
		return glm::normalize(
			direction_local.x * x_axis +
			direction_local.y * y_axis +
			direction_local.z * z_axis
		);
	}


private:
	glm::vec3 x_axis, y_axis, z_axis;
};