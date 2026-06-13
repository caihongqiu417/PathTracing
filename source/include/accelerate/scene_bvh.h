#pragma once

#include"bounds.h"
#include"shape/shape.h"

// ============================================================
// SceneBVH —— 场景级加速结构（第一层）
//
// 职责：加速"光线 vs 场景中所有物体实例"的求交
//
// 构建流程（Build 时执行一次）：
//   ShapeInstance[] → 分流（有限 / 无限大）
//   有限实例 → RecursiveSplit → SceneBVHTreeNode 树
//            → RecursiveFlatten → SceneBVHNode[] 线性数组
//   无限大实例（平面等）→ infinity_instances[]（每条光线都要测）
//
// 查询流程（每条光线执行）：
//   Intersect → 遍历 SceneBVHNode[]
//             → 包围盒剔除 → ShapeInstance::Intersect
//             → 补测 infinity_instances
// ============================================================


// ----------------------------------------------------------------
// [数据] 场景中一个物体的摆放
// ----------------------------------------------------------------

// 同一个 Shape 可以用不同变换矩阵摆在场景的不同位置，每个摆放就是一个 ShapeInstance
struct ShapeInstance
{
	const Shape& shape;
	Material material;  // 按值保存，避免传入临时对象时产生悬空引用

	glm::mat4 world_from_object;  // 模型空间 → 世界空间
	glm::mat4 object_from_world;  // 世界空间 → 模型空间

	Bounds    bounds  = Bounds{};   // 世界空间包围盒（Build 前调用 UpdateBounds 填好）
	glm::vec3 center  = glm::vec3(0);  // 包围盒中心（SAH 分桶时用）

	// 把模型空间包围盒的 8 个顶角变换到世界空间，求外接包围盒
	void UpdateBounds()
	{
		bounds = {};
		auto bounds_object = shape.GetBounds();
		for (size_t idx = 0; idx < 8; idx++)
		{
			glm::vec3 corner_world = world_from_object * glm::vec4(bounds_object.GetCorner(idx), 1.f);
			bounds.Expand(corner_world);
		}
		center = (bounds.b_max + bounds.b_min) * 0.5f;
	}

	// 把光线变换到模型空间求交，再把结果变换回世界空间
	std::optional<HitInfo> Intersect(const Ray& ray, float t_min, float t_max) const
	{
		Ray ray_object;
		ray_object.origin    = glm::vec3(object_from_world * glm::vec4(ray.origin,    1.f));
		ray_object.direction = glm::vec3(object_from_world * glm::vec4(ray.direction, 0.f));

		auto hit = shape.Intersect(ray_object, t_min, t_max);
		if (!hit) return std::nullopt;

		hit->point    = glm::vec3(world_from_object * glm::vec4(hit->point, 1.f));
		hit->normal   = glm::normalize(glm::mat3(glm::transpose(object_from_world)) * hit->normal);  // 法线用逆转置变换
		hit->material = &material;
		return hit;
	}
};


// ----------------------------------------------------------------
// [构建期] 树节点：仅在 Build() 过程中存在，构建完成后释放
// ----------------------------------------------------------------

struct SceneBVHTreeNode
{
	Bounds bounds{};
	std::vector<ShapeInstance> instances;  // 叶子节点才有，内部节点构建完后清空
	SceneBVHTreeNode* children[2] = { nullptr, nullptr };
	size_t depth;
	size_t split_axis;  // 沿哪个轴切割（0=x 1=y 2=z）

	void UpdateBounds()
	{
		bounds = {};
		for (const auto& instance : instances)
			bounds.Expand(instance.bounds);
	}
};

// 内存池：一次申请 4096 个节点，避免大量零散 new，构建结束时析构整块释放
class SceneBVHTreeNodeAllocator
{
public:
	SceneBVHTreeNodeAllocator() : ptr(4096) {}  // 初始化为"已满"，第一次 allocate 就触发申请

	SceneBVHTreeNode* allocate()
	{
		if (ptr == 4096)
		{
			nodes_list.push_back(new SceneBVHTreeNode[4096]);
			ptr = 0;
		}
		return &(nodes_list.back()[ptr++]);
	}

	~SceneBVHTreeNodeAllocator()
	{
		for (auto* nodes : nodes_list)
			delete[] nodes;
	}

private:
	size_t ptr;
	std::vector<SceneBVHTreeNode*> nodes_list;
};

// 构建过程统计，Build 完成后打印
struct SceneBVHState
{
	size_t total_node_count = 0;
	size_t leaf_node_count = 0;
	size_t max_leaf_node_instance_count = 0;

	void AddLeafNode(SceneBVHTreeNode* node)
	{
		leaf_node_count++;
		max_leaf_node_instance_count = glm::max(max_leaf_node_instance_count, node->instances.size());
	}
};


// ----------------------------------------------------------------
// [渲染期] 线性节点：Build 完成后存入数组，渲染时遍历
// ----------------------------------------------------------------

struct alignas(32) SceneBVHNode
{
	Bounds bounds{};

	union
	{
		int child1_index;    // 内部节点：右子节点下标（左子节点隐式为当前下标+1）
		int instance_index;  // 叶子节点：实例在 ordered_instances 中的起始下标
	};

	uint16_t instance_count;  // 叶子节点的实例数量（内部节点为 0）
	uint8_t  depth;
	uint8_t  split_axis;      // 遍历时按此轴决定先走左子还是右子
};


// ----------------------------------------------------------------
// [对外接口] SceneBVH 整体
// ----------------------------------------------------------------

class SceneBVH : public Shape
{
public:
	void Build(std::vector<ShapeInstance>&& instances);

	std::optional<HitInfo> Intersect(const Ray& ray, float t_min, float t_max) const override;

	Bounds GetBounds() const override { return nodes.empty() ? Bounds{} : nodes[0].bounds; }

private:
	void   DeleteNode(SceneBVHTreeNode* node);
	void   RecursiveSplit(SceneBVHTreeNode* node, SceneBVHState& state, SceneBVHTreeNodeAllocator& allocator);
	size_t RecursiveFlatten(SceneBVHTreeNode* node);

private:
	std::vector<SceneBVHNode>   nodes;              // 线性 BVH 数组，遍历时使用
	std::vector<ShapeInstance>  ordered_instances;  // 按叶子顺序排列的有限实例列表
	std::vector<ShapeInstance>  infinity_instances; // 无限大物体（如平面），每条光线都要测
};
