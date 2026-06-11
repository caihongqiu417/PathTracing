#pragma once

#include "triangle.h"

#include"accelerate/bvh.h"
#include <filesystem>
#include <vector>

class Model :public Shape
{
public:
	Model(const std::vector<Triangle>& triangles)
	{
		//构建BVH树
		auto ts = triangles;
		bvh.Build(std::move(ts));
	}

	//从文件加载（obj)
	Model(const std::filesystem::path& filename);


	//求交点
	std::optional<HitInfo> Intersect(const Ray& ray, float t_min, float t_max)const override;

private:
	BVH bvh{};		//// BVH结构在构造函数末尾生成，会接管所有三角形！！！
};