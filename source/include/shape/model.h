#pragma once

#include "triangle.h"

#include"accelerate/bounds.h"
#include <filesystem>
#include <vector>

class Model :public Shape
{
public:
	Model(const std::vector<Triangle>& triangles)
		:triangles(triangles)
	{
		//AABB生成
		Build();
	}

	//从文件加载（obj)
	Model(const std::filesystem::path& filename);


	//求交点
	std::optional<HitInfo> Intersect(const Ray& ray, float t_min, float t_max)const override;

private:
	//根据模型生成包围盒（构造函数末尾调用）
	void Build();
private:
	std::vector<Triangle> triangles; //模型含的所有三角形
	Bounds bounds{};				//模型的包围盒
};