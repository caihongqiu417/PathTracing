// ============================================================
// SceneBVH —— 场景级加速结构
//
// 管理对象：场景里的所有物体实例（ShapeInstance）
// 工作空间：世界空间（实例有各自的变换矩阵）
// 质心来源：预存在 ShapeInstance::center（UpdateBounds 时算好）
// 叶子存的：ShapeInstance
//
// 调用链：光线（世界空间）进入 SceneBVH
//         → 找到候选 ShapeInstance
//         → ShapeInstance::Intersect 把光线变到模型空间
//         → 交给内层 BVH 或 Shape 做精确求交
// ============================================================
#include "scene_bvh.h"
#include"debug_macro.h"
#include<array>
#include<iostream>


//===========================================================================================
// 构建 SceneBVH
// 流程：1. 创建根节点并接管所有物体实例
//       2. 计算根节点的包围盒
//       3. 递归分割节点，直到每个叶子节点只含少量实例
//       4. 线性化（树结构 => 数组结构，便于遍历时缓存友好）
// 注意：分割后只有叶子节点存储实例，内部节点的实例列表会被清空
//===========================================================================================
void SceneBVH::Build(std::vector<ShapeInstance>&& instances)
{
	// 无限大物体（如平面）没有有意义的包围盒，放进 BVH 会让包围盒退化。
	// 判断标准：包围盒面积为 0（所有顶角变换后重合成一个点）。
	// 这类实例单独存入 infinity_instances，每条光线都必须与它们逐一测试。
	std::vector<ShapeInstance> finite_instances;
	for (auto& instance : instances)
	{
		if (instance.bounds.Area() < 1e-6f)
			infinity_instances.push_back(std::move(instance));
		else
			finite_instances.push_back(std::move(instance));
	}

	std::cout << "[SceneBVH] 无限大实例数:   " << infinity_instances.size() << "\n";

	if (finite_instances.empty()) return;  // 场景里全是无限大物体，不需要建 BVH

	SceneBVHTreeNodeAllocator allocator;

	auto root = allocator.allocate();
	root->instances = std::move(finite_instances);
	root->UpdateBounds();
	root->depth = 1;

	SceneBVHState state{};
	RecursiveSplit(root, state, allocator);

	std::cout << "[SceneBVH] 总节点数:       " << state.total_node_count              << "\n"
	          << "[SceneBVH] 叶子节点数:     " << state.leaf_node_count               << "\n"
	          << "[SceneBVH] 叶子最大实例数: " << state.max_leaf_node_instance_count  << "\n";

	RecursiveFlatten(root);
}



void SceneBVH::RecursiveSplit(SceneBVHTreeNode* node, SceneBVHState& state, SceneBVHTreeNodeAllocator& allocator)
{
	state.total_node_count++;

	// -----------------------------------------------------------------------
	// 1. 递归出口
	// -----------------------------------------------------------------------
	if (node->instances.size() == 1 || node->depth > 32)
	{
		state.AddLeafNode(node);
		return;
	}

	// -----------------------------------------------------------------------
	// 2. SAH 桶算法，找到最佳切割方式
	//
	//    目标：把这个节点的实例分成左右两堆，让光线遍历最快。
	//
	//    评分标准：左边的"包围盒面积 × 实例数"加上右边的"包围盒面积 × 实例数"
	//             这个值越小越好——盒子越小、实例越少，光线测试的工作量就越少。
	//
	//    怎么找：沿 x / y / z 三个方向分别试，每个方向把包围盒均匀切成 12 段（桶），
	//           每段收集落在里面的实例，然后逐一尝试在哪两段之间切一刀得分最低。
	//           共 3 × 11 = 33 种切法，选得分最低的那个。
	// -----------------------------------------------------------------------
	constexpr size_t bucket_count = 12;

	const float     parent_area = node->bounds.Area();
	const glm::vec3 diag        = node->bounds.Diagonal();

	float  min_cost    = std::numeric_limits<float>::infinity();
	size_t best_axis   = 0;
	size_t best_bucket = 0;  // 分割点：桶[0..k] 归左子，桶[k+1..末尾] 归右子

	// 给定实例中心在某轴上的坐标，算出它落在哪个桶（0 ~ bucket_count-1）
	auto bucket_of = [&](float centroid, size_t axis) -> size_t {
		float t = (centroid - node->bounds.b_min[axis]) / diag[axis];
		int   i = static_cast<int>(t * (float)bucket_count);
		return static_cast<size_t>(glm::clamp(i, 0, (int)bucket_count - 1));
	};

	for (size_t axis = 0; axis < 3; axis++)
	{
		if (diag[axis] < 1e-6f) continue;  // 该轴所有实例重叠，切了没意义

		// 初始化每个桶的包围盒为空（+inf/-inf 保证第一次 Expand 正确）
		Bounds bucket_bounds[bucket_count];
		size_t bucket_cnt[bucket_count] = {};
		for (auto& b : bucket_bounds)
		{
			b.b_min = glm::vec3( std::numeric_limits<float>::max());
			b.b_max = glm::vec3(-std::numeric_limits<float>::max());
		}

		// 把每个实例按中心位置丢进对应的桶，并扩展该桶的包围盒
		for (const auto& instance : node->instances)
		{
			size_t idx = bucket_of(instance.center[axis], axis);
			bucket_bounds[idx].Expand(instance.bounds);
			bucket_cnt[idx]++;
		}

		// 逐一试验每个切点：切在 k 和 k+1 之间，左子 = 桶[0..k]，右子 = 桶[k+1..末尾]
		for (size_t k = 0; k < bucket_count - 1; k++)
		{
			// 合并左边所有桶的包围盒
			Bounds left_bounds;
			left_bounds.b_min = glm::vec3( std::numeric_limits<float>::max());
			left_bounds.b_max = glm::vec3(-std::numeric_limits<float>::max());
			size_t left_count = 0;
			for (size_t i = 0; i <= k; i++)
			{
				if (bucket_cnt[i] > 0)
				{
					left_bounds.Expand(bucket_bounds[i]);
					left_count += bucket_cnt[i];
				}
			}

			// 合并右边所有桶的包围盒
			Bounds right_bounds;
			right_bounds.b_min = glm::vec3( std::numeric_limits<float>::max());
			right_bounds.b_max = glm::vec3(-std::numeric_limits<float>::max());
			size_t right_count = 0;
			for (size_t i = k + 1; i < bucket_count; i++)
			{
				if (bucket_cnt[i] > 0)
				{
					right_bounds.Expand(bucket_bounds[i]);
					right_count += bucket_cnt[i];
				}
			}

			if (left_count == 0 || right_count == 0) continue;

			// 得分 = 左边包围盒面积 × 左边实例数 + 右边包围盒面积 × 右边实例数
			float cost = (left_bounds.Area()  * (float)left_count
			            + right_bounds.Area() * (float)right_count)
			            / parent_area;

			if (cost < min_cost)
			{
				min_cost    = cost;
				best_axis   = axis;
				best_bucket = k;
			}
		}
	}

	// -----------------------------------------------------------------------
	// 3. 用选出的最佳切法，把实例分进左右两个子节点
	// -----------------------------------------------------------------------
	std::vector<ShapeInstance> child0_instances, child1_instances;
	for (const auto& instance : node->instances)
	{
		size_t idx = bucket_of(instance.center[best_axis], best_axis);
		if (idx <= best_bucket)
			child0_instances.push_back(instance);
		else
			child1_instances.push_back(instance);
	}

	// 极端情况：所有实例落在同一侧，无法继续分割，强制作为叶子节点
	if (child0_instances.empty() || child1_instances.empty())
	{
		state.AddLeafNode(node);
		return;
	}

	// -----------------------------------------------------------------------
	// 4. 创建子节点并递归
	// -----------------------------------------------------------------------
	auto* child0 = allocator.allocate();
	auto* child1 = allocator.allocate();
	node->children[0] = child0;
	node->children[1] = child1;
	node->split_axis   = best_axis;

	node->instances.clear();         // 内部节点不存实例
	node->instances.shrink_to_fit();

	child0->depth = node->depth + 1;
	child1->depth = node->depth + 1;
	child0->instances = std::move(child0_instances);
	child1->instances = std::move(child1_instances);
	child0->UpdateBounds();
	child1->UpdateBounds();

	RecursiveSplit(child0, state, allocator);
	RecursiveSplit(child1, state, allocator);
}



//===========================================================================================
// 相交检测：遍历 SceneBVH 线性数组，找到光线击中的最近实例
//===========================================================================================
std::optional<HitInfo> SceneBVH::Intersect(const Ray& ray, float t_min, float t_max) const
{
	std::optional<HitInfo> closest_hit_info;

	DEBUG_LINE(size_t bounds_test_count = 0, instance_test_count = 0;)

	// 光线方向是否为负（用于决定先遍历哪个子节点，让更近的子节点先测试）
	glm::bvec3 dir_is_neg = {
		ray.direction.x < 0,
		ray.direction.y < 0,
		ray.direction.z < 0
	};

	glm::vec3 inv_direction = 1.f / ray.direction;  // 预计算倒数，避免重复除法

	std::array<int, 32> stack;   // 待处理节点的下标栈
	auto ptr = stack.begin();

	size_t current_node_index = 0;

	while (true)
	{
		auto& node = nodes[current_node_index];

		DEBUG_LINE(bounds_test_count++;)

		// a. 光线与当前节点包围盒无交点，跳过整个子树
		if (!node.bounds.HasIntersection(ray, inv_direction, t_min, t_max))
		{
			if (ptr == stack.begin()) break;       // 栈空，遍历结束
			current_node_index = *(--ptr);         // 取出下一个待处理节点
			continue;
		}

		// b. 光线与当前节点包围盒有交点
		if (node.instance_count == 0)
		{
			// b1. 内部节点：决定左右子树的遍历顺序
			// 光线方向为负时，右子节点更近，先遍历右子；否则先遍历左子
			if (dir_is_neg[node.split_axis])
			{
				*(ptr++) = node.child1_index;  // 右子压栈（稍后处理）
				current_node_index++;          // 进入左子（紧跟在当前节点后面）
			}
			else
			{
				*(ptr++) = node.child1_index;  // 右子压栈（稍后处理）
				current_node_index++;          // 进入左子
			}
		}
		else
		{
			// b2. 叶子节点：逐一检测节点内的每个实例
			auto instance_iter = ordered_instances.begin() + node.instance_index;
			DEBUG_LINE(instance_test_count += node.instance_count;)
			for (size_t i = 0; i < node.instance_count; i++, ++instance_iter)
			{
				auto hit_info = instance_iter->Intersect(ray, t_min, t_max);
				if (hit_info)
				{
					t_max = hit_info->t;  // 缩短有效范围，后续实例必须比这个更近才算
					closest_hit_info = hit_info;
					DEBUG_LINE(closest_hit_info->bounds_depth = node.depth;)
				}
			}
			if (ptr == stack.begin()) break;
			current_node_index = *(--ptr);
		}
	}

	// 无限大物体没有包围盒，BVH 遍历跳过了它们，这里单独补测
	for (const auto& instance : infinity_instances)
	{
		DEBUG_LINE(instance_test_count++;)
		auto hit_info = instance.Intersect(ray, t_min, t_max);
		if (hit_info)
		{
			t_max = hit_info->t;
			closest_hit_info = hit_info;
		}
	}

	if (closest_hit_info.has_value())
	{
		DEBUG_LINE(closest_hit_info->bounds_test_count   = bounds_test_count;)
		DEBUG_LINE(closest_hit_info->instance_test_count = instance_test_count;)
	}

	return closest_hit_info;
}



//===========================================================================================
// 线性化：把构建期的树结构转成数组，方便遍历时连续访问内存
// 采用 DFS 前序遍历：当前节点先存，然后递归左子、右子
// 左子紧跟在当前节点后面（下标隐式为 current+1），右子下标显式存储
//===========================================================================================
size_t SceneBVH::RecursiveFlatten(SceneBVHTreeNode* node)
{
	SceneBVHNode linear_node{
		node->bounds,
		0,
		static_cast<uint16_t>(node->instances.size()),
		static_cast<uint8_t>(node->depth),
		static_cast<uint8_t>(node->split_axis)
	};

	auto idx = nodes.size();
	nodes.push_back(linear_node);

	if (linear_node.instance_count == 0)
	{
		// 内部节点：左子紧跟在后面（不需要记录），右子下标显式存储
		RecursiveFlatten(node->children[0]);
		nodes[idx].child1_index = RecursiveFlatten(node->children[1]);
	}
	else
	{
		// 叶子节点：把实例追加到全局列表，记录起始下标
		nodes[idx].instance_index = ordered_instances.size();
		for (const auto& instance : node->instances)
			ordered_instances.push_back(instance);
	}

	return idx;
}
