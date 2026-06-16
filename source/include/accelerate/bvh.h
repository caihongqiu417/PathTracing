#pragma once

#include"bounds.h"
#include"shape/triangle.h"
#include"sample/alias_table.h"

// ============================================================
// BVH —— 模型内部加速结构（第二层）
//
// 职责：加速"光线 vs 单个模型内所有三角形"的求交
//
// 构建流程（Build 时执行一次）：
//   Triangle[] → RecursiveSplit → BVHTreeNode 树
//              → RecursiveFlatten → BVHNode[] 线性数组
//
// 查询流程（每条光线执行）：
//   Intersect → 遍历 BVHNode[]
//             → 包围盒剔除 → Triangle::Intersect
// ============================================================


// ----------------------------------------------------------------
// [构建期] 树节点：仅在 Build() 过程中存在，构建完成后释放
// ----------------------------------------------------------------

struct BVHTreeNode
{
	Bounds bounds{};
	std::vector<Triangle> triangles;    // 叶子节点才有，内部节点构建完后清空
	BVHTreeNode* children[2] = { nullptr, nullptr };
	size_t depth;
	size_t split_axis;  // 沿哪个轴切割（0=x 1=y 2=z）

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

// 内存池：一次申请 4096 个节点，避免大量零散 new
// 构建结束时析构，整块释放
class BVHTreeNodeAllocator
{
public:
	BVHTreeNodeAllocator() : ptr(4096) {}  // 初始化为"已满"，第一次 allocate 就触发申请

	BVHTreeNode* allocate()
	{
		if (ptr == 4096)
		{
			nodes_list.push_back(new BVHTreeNode[4096]);
			ptr = 0;
		}
		return &(nodes_list.back()[ptr++]);
	}

	~BVHTreeNodeAllocator()
	{
		for (auto* nodes : nodes_list)
			delete[] nodes;
	}

private:
	size_t ptr;
	std::vector<BVHTreeNode*> nodes_list;
};

// 构建过程统计，Build 完成后打印
struct BVHState {
	size_t total_node_count = 0;
	size_t leaf_node_count = 0;
	size_t max_leaf_node_triangle_count = 0;

	void AddLeafNode(BVHTreeNode* node)
	{
		leaf_node_count++;
		max_leaf_node_triangle_count = glm::max(max_leaf_node_triangle_count, node->triangles.size());
	}
};


// ----------------------------------------------------------------
// [渲染期] 线性节点：Build 完成后存入数组，渲染时遍历
// ----------------------------------------------------------------

struct alignas(32) BVHNode
{
	Bounds bounds{};

	union
	{
		int child1_index;    // 内部节点：右子节点下标（左子节点隐式为当前下标+1）
		int triangle_index;  // 叶子节点：三角形在 ordered_triangles 中的起始下标
	};

	uint16_t triangle_count;  // 叶子节点的三角形数量（内部节点为 0）
	uint8_t  depth;
	uint8_t  split_axis;      // 遍历时按此轴决定先走左子还是右子
};


// ----------------------------------------------------------------
// [对外接口] BVH 整体
// ----------------------------------------------------------------

class BVH : public Shape
{
public:
	void Build(std::vector<Triangle>&& triangles);

	std::optional<HitInfo> Intersect(const Ray& ray, float t_min, float t_max) const override;

	Bounds GetBounds() const override { return nodes.empty() ? Bounds{} : nodes[0].bounds; }

	float GetArea()const override { return area; }

	//
	virtual std::optional<ShapeSample> SampleShape(Rng& rng)const override;

private:
	void   DeleteNode(BVHTreeNode* node);
	void   RecursiveSplit(BVHTreeNode* node, BVHState& state, BVHTreeNodeAllocator& allocator);
	size_t RecursiveFlatten(BVHTreeNode* node);

private:
	std::vector<BVHNode>   nodes;             // 线性 BVH 数组，遍历时使用
	std::vector<Triangle>  ordered_triangles; // 按叶子顺序排列的三角形列表

	float area;
	AliasTable alias_table;
};
