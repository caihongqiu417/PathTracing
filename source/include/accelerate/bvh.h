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

/// @brief BVHTreeNode 内存池分配器
///
/// 问题：BVH 构建时需要创建大量 BVHTreeNode，如果每次都用 new，
///       会产生大量零散的系统调用，内存碎片化，效率低。
///
/// 解决：一次性向系统申请一大块内存（4096 个节点），
///       在块内用下标线性分配，块满了再申请下一块。
///       析构时统一释放，不需要逐个 delete。
///
/// 内存结构：
///   nodes_list[0] → [node0][node1][node2]...[node4095]
///   nodes_list[1] → [node0][node1][node2]...[node4095]  ← 第一块满了再申请
///                                 ↑
///                                ptr（当前块用到哪了，0~4095）
class BVHTreeNodeAllocator
{
public:
	// ptr 故意初始化为 4096（已满状态），
	// 让第一次 allocate() 就触发新块申请，不需要在构造函数里预申请
	BVHTreeNodeAllocator() : ptr(4096) {}

	BVHTreeNode* allocate()
	{
		if (ptr == 4096) // 当前块已满（或尚未申请），申请新块
		{
			nodes_list.push_back(new BVHTreeNode[4096]);
			ptr = 0; // 从新块的第 0 个开始分配
		}
		return &(nodes_list.back()[ptr++]); // 返回当前位置，ptr 后移一位
	}

	// Build() 结束时 allocator 出作用域，析构自动释放整棵构建树
	~BVHTreeNodeAllocator()
	{
		for (auto* nodes : nodes_list)
			delete[] nodes; // 整块释放，比逐个 delete 快得多
	}

private:
	size_t ptr;                           // 当前块内下一个可用位置（0~4095）
	std::vector<BVHTreeNode*> nodes_list; // 所有已申请的块
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
	void RecursiveSplit(BVHTreeNode* node, BVHState& state, BVHTreeNodeAllocator& allocator);

	//将BVH转化成线性结构
	size_t RecursiveFlatten(BVHTreeNode* node);

private:
	//存储BVH转化后的线性结构
	std::vector<BVHNode> nodes;

	//总的三角形列表(供BVH节点访问)
	std::vector<Triangle> ordered_triangles;
};