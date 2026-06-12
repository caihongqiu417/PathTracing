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
	BVHTreeNode* children[2] = { nullptr, nullptr };

	size_t depth;

	//划分轴
	size_t split_axis;

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
	
	uint16_t triangle_count;
	uint8_t depth;

	uint8_t split_axis;  //划分的轴，用于决定左右子树的遍历顺序

};

/// @brief BVH树的统计信息，用于调试和性能分析
/// RecursiveSplit 构建过程中实时填写，Build 完成后打印
struct BVHState {
	size_t total_node_count = 0;             // 总节点数（内部节点 + 叶子节点）
	size_t leaf_node_count = 0;              // 叶子节点数（每个叶子对应一组三角形）
	size_t max_leaf_node_triangle_count = 0; // 单个叶子节点最多包含的三角形数

	// 每遇到一个叶子节点时调用，更新叶子相关统计
	void AddLeafNode(BVHTreeNode* node)
	{
		leaf_node_count++;
		max_leaf_node_triangle_count = glm::max(max_leaf_node_triangle_count, node->triangles.size());
	}
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
	void RecursiveSplit(BVHTreeNode* node,BVHState& state);

	//将BVH转化成线性结构
	size_t RecursiveFlatten(BVHTreeNode* node);

private:
	//存储BVH转化后的线性结构
	std::vector<BVHNode> nodes;

	//总的三角形列表(供BVH节点访问)
	std::vector<Triangle> ordered_triangles;
};