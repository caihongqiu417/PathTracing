#pragma once

#include"bounds.h"
#include"shape/triangle.h"

/// @brief BVH树的一个节点
struct BVHNode
{
	//AABB包围盒
	Bounds bounds{};

	//节点含有的三角形
	std::vector<Triangle> triangles;

	//左右孩子BVH节点
	BVHNode* children[2] = {nullptr, nullptr};

	//遍历这个节点的所有三角形并更新节点的包围盒
	void UpdateBounds()
	{
		bounds = {};
		for (const auto& triangle : triangles)
		{
			bounds.Expand(triangle.p0);
			bounds.Expand(triangle.p1);
			bounds.Expand(triangle.p2);
		}
	}
};

/// @brief 整个BVH树
class BVH:public Shape
{
public:
	BVH() : root(nullptr) {}
	~BVH();

	//从三角形开始构造BVH树
	void Build(std::vector<Triangle>&& triangles);

	//相交检测
	std::optional<HitInfo> Intersect(const Ray& ray, float t_min, float t_max)const override;
private:
	//递归删除BVH树节点
	void DeleteNode(BVHNode* node);

	//递归分割BVH
	void RecursiveSplit(BVHNode* node);

	//递归相交检测
	void RecursiveIntersect(
		BVHNode* node,
		const Ray& ray, float t_min, float& t_max,
		std::optional<HitInfo>& closest_hit_info
	)const;
private:
	BVHNode* root;
};