#pragma once

#include <glm/glm.hpp>

struct Material
{
	//反照率
	glm::vec3 albedo = { 1,1,1 };

	//漫反射/镜面反射
	bool is_specular = false;

	//自发光
	glm::vec3 emissive = { 0,0,0 };
};