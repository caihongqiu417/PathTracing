#pragma once

#include"bounds.h"
#include"shape/triangle.h"

/// @brief BVH树的一个节点
struct BVHTreeNode
{
	//AABB包围盒
	Bounds bounds{};

	//节点含有的三角形
	std::vector<Triangle> triangles;

	//左右孩子BVH节点
	BVHTreeNode* children[2] = {nullptr, nullptr};

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

//	BVH数组的节点
struct alignas(32) BVHNode
{
	//包围盒
	Bounds bounds{};

	union
	{
		//右孩子索引（内部节点用）
		int child1_index;

		//三角形索引（叶子节点用)
		int triangle_index;
	};
	
	int triangle_count;

};

/// @brief 整个BVH树
class BVH :public Shape
{
public:
	//从三角形开始构造BVH树
	void Build(std::vector<Triangle>&& triangles);

	//相交检测
	std::optional<HitInfo> Intersect(const Ray& ray, float t_min, float t_max)const override;
private:
	//递归删除BVH树节点
	void DeleteNode(BVHTreeNode* node);

	//递归分割BVH
	void RecursiveSplit(BVHTreeNode* node);

	//将BVH转化成线性结构
	size_t RecursiveFlatten(BVHTreeNode* node);

private:
	//存储BVH转化后的线性结构
	std::vector<BVHNode> nodes;

	//总的三角形列表(供BVH节点访问)
	std::vector<Triangle> ordered_triangles;
};