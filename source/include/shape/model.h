#pragma once

#include "triangle.h"

#include <filesystem>
#include <vector>

class Model :public Shape
{
public:
	Model(const std::vector<Triangle>& triangles)
		:triangles(triangles) {
	}

	//从文件加载（obj)
	Model(const std::filesystem::path& filename);


	//求交点
	std::optional<HitInfo> Intersect(const Ray& ray, float t_min, float t_max)const override;
private:
	std::vector<Triangle> triangles;
};