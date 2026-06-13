// ============================================================
//	BVH —— 模型级加速结构
//
// 管理对象：一个模型内部的所有三角形
// 工作空间：模型空间（顶点坐标直接使用，不需要变换）
// 质心来源：三角形三个顶点的平均值 (p0+p1+p2)/3
// 叶子存的：Triangle
//
// 调用链：SceneBVH 找到候选 ShapeInstance
//         → ShapeInstance 把光线变换到模型空间
//         → BVH::Intersect 在模型内部找三角形交点
// ============================================================
#include "bvh.h"
#include"debug_macro.h"
#include<array>
#include<iostream>


//===========================================================================================
// 构建BVH
// 功能：三角形列表=>BVH树结构=>BVH线性结构
// 流程：1. 创建根节点并接管所有三角形
//       2. 计算根节点的包围盒
//       3. 递归分割节点，直到每个叶子节点包含少量三角形
//		 4. 线性化
// 注意：分割后，只有叶子节点才存储三角形，内部节点三角形列表会清空
//===========================================================================================
void BVH::Build(std::vector<Triangle>&& triangles)
{
	// 创建内存池，构建期间所有 BVHTreeNode 从这里分配
	// Build 结束时 allocator 析构，自动释放整棵树
	BVHTreeNodeAllocator allocator;

	//三角形列表=>BVH树结构
	auto root = allocator.allocate();
	root->triangles = std::move(triangles);
	root->UpdateBounds();

	//根节点深度设置为1
	root->depth = 1;

	BVHState state{};
	RecursiveSplit(root, state, allocator);

	std::cout << "[BVH] 总节点数:         " << state.total_node_count             << "\n"
	          << "[BVH] 叶子节点数:       " << state.leaf_node_count              << "\n"
	          << "[BVH] 叶子最大三角形数: " << state.max_leaf_node_triangle_count << "\n";

	//BVH树结构=>BVH线性结构
	RecursiveFlatten(root);
}



void BVH::RecursiveSplit(BVHTreeNode* node, BVHState& state, BVHTreeNodeAllocator& allocator)
{
	state.total_node_count++;

	// -----------------------------------------------------------------------
	// 1. 递归出口
	// -----------------------------------------------------------------------
	if (node->triangles.size() == 1 || node->depth > 32)
	{
		state.AddLeafNode(node);
		return;
	}

	// -----------------------------------------------------------------------
	// 2. 找到最佳切割方式
	//
	//    目标：把这个节点的三角形分成左右两堆，让光线遍历最快。
	//
	//    评分标准：分完之后，左边的"盒子面积 × 三角形数"加上右边的"盒子面积 × 三角形数"
	//             这个值越小越好——盒子越小、三角形越少，光线测试的工作量就越少。
	//
	//    怎么找：沿 x / y / z 三个方向分别试，每个方向把包围盒均匀切成 12 段（桶），
	//           每段收集落在里面的三角形，然后逐一尝试在哪两段之间切一刀得分最低。
	//           共 3 × 11 = 33 种切法，选得分最低的那个。
	//
	//    加速技巧：从左往右扫一遍算出"前 k 段"的合并包围盒，
	//             再从右往左扫一遍算出"后 k 段"的合并包围盒，
	//             这样枚举每种切法时直接查表，不用重复计算。
	// -----------------------------------------------------------------------
	constexpr size_t bucket_count = 12;  // 每个方向划分的段数

	const float     parent_area = node->bounds.Area();  // 当前节点包围盒的表面积（用于归一化得分）
	const glm::vec3 diag        = node->bounds.Diagonal();  // 包围盒在三个轴上的长度

	float  min_cost   = std::numeric_limits<float>::infinity();  // 记录目前最低得分
	size_t best_axis   = 0;   // 最佳切割轴（0=x 1=y 2=z）
	size_t best_bucket = 0;   // 最佳切割位置：第 best_bucket 段和第 best_bucket+1 段之间

	// 给定三角形质心在某轴上的坐标，算出它落在哪个桶（0 ~ bucket_count-1）
	auto bucket_of = [&](float centroid, size_t axis) -> size_t {
		// 先把质心归一化到 [0, 1]，再乘以桶数，得到桶下标
		float t = (centroid - node->bounds.b_min[axis]) / diag[axis];
		int   i = static_cast<int>(t * (float)bucket_count);
		return static_cast<size_t>(glm::clamp(i, 0, (int)bucket_count - 1));  // 防止越界
	};

	for (size_t axis = 0; axis < 3; axis++)
	{
		if (diag[axis] < 1e-6f) continue;

		// 初始化每个桶的包围盒为空
		Bounds bucket_bounds[bucket_count];
		size_t bucket_cnt[bucket_count] = {}; 
		for (auto& b : bucket_bounds)
		{
			b.b_min = glm::vec3( std::numeric_limits<float>::max());
			b.b_max = glm::vec3(-std::numeric_limits<float>::max());
		}

		// 把每个三角形按质心位置丢进对应的桶，同时把三角形的三个顶点纳入该桶的包围盒
		for (const auto& tri : node->triangles)
		{
			//计算三角形质心(三角形中点在x轴的位置)
			float  centroid = (tri.p0[axis] + tri.p1[axis] + tri.p2[axis]) / 3.f;
			//计算这个质心对应哪个桶
			size_t idx = bucket_of(centroid, axis);
			//把三角形的三个顶点纳入该桶的包围盒
			bucket_bounds[idx].Expand(tri.p0);
			bucket_bounds[idx].Expand(tri.p1);
			bucket_bounds[idx].Expand(tri.p2);
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
					left_bounds.Expand(bucket_bounds[i].b_min);
					left_bounds.Expand(bucket_bounds[i].b_max);
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
					right_bounds.Expand(bucket_bounds[i].b_min);
					right_bounds.Expand(bucket_bounds[i].b_max);
					right_count += bucket_cnt[i];
				}
			}

			if (left_count == 0 || right_count == 0) continue;

			// cost = 左边包围盒面积 × 左边三角形数 + 右边包围盒面积 × 右边三角形数
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
	// 3. 用选出的最佳切法，把三角形分进左右两个子节点
	// -----------------------------------------------------------------------
	std::vector<Triangle> child0_triangles, child1_triangles;
	for (const auto& tri : node->triangles)
	{
		float  centroid = (tri.p0[best_axis] + tri.p1[best_axis] + tri.p2[best_axis]) / 3.f;
		size_t idx      = bucket_of(centroid, best_axis);
		if (idx <= best_bucket)
			child0_triangles.push_back(tri);
		else
			child1_triangles.push_back(tri);
	}

	// 所有三角形仍落在同一侧，强制作为叶子节点
	if (child0_triangles.empty() || child1_triangles.empty())
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

	node->triangles.clear();        // 内部节点不存三角形
	node->triangles.shrink_to_fit();

	child0->depth = node->depth + 1;
	child1->depth = node->depth + 1;
	child0->triangles = std::move(child0_triangles);
	child1->triangles = std::move(child1_triangles);
	child0->UpdateBounds();
	child1->UpdateBounds();

	RecursiveSplit(child0, state, allocator);
	RecursiveSplit(child1, state, allocator);
}



//===========================================================================================
// 相交检测:遍历BVH结构，找到所有与光线相交的三角形
//===========================================================================================
std::optional<HitInfo> BVH::Intersect(const Ray& ray, float t_min, float t_max) const
{

	std::optional<HitInfo> closest_hit_info;

	DEBUG_LINE(size_t bounds_test_count = 0, triangle_test_count = 0;)

		//判断光线的方向(用于判断左右子树的遍历顺序)
		glm::bvec3 dir_is_neg = {
		ray.direction.x < 0,
		ray.direction.y < 0,
		ray.direction.z < 0
	};

	//预先计算光线倒数（用于优化查找速率）
	glm::vec3 inv_direction = 1.f / ray.direction;

	//栈：代表待办节点
	std::array<int, 32> stack;
	auto ptr = stack.begin();

	//索引：代表当前遍历的节点
	size_t current_node_index = 0;

	while (true)
	{
		auto& node = nodes[current_node_index];
		
		DEBUG_LINE(bounds_test_count++;)

		// -- a.如果光线和当前节点没有交点 --
		if (!node.bounds.HasIntersection(ray,inv_direction,t_min, t_max))
		{

			if (ptr == stack.begin()) break; //a1 栈空：没有待办节点=>退出循环
			current_node_index = *(--ptr);   //a2 栈不空：处理下一个待办节点=>下一轮循环
			continue;
		}

		// -- b.如果光线和当前节点有交点 --
		if (node.triangle_count==0)
		{
			// b1 内部节点
			if (dir_is_neg[node.split_axis])	//如果光线在分割轴是负的=>则先右再左
			{
				*(ptr++) = node.child1_index;	//压栈右子树
				current_node_index++; 			//进左子树
			}
			else {
				current_node_index++; 			//进左子树
				*(ptr++) = node.child1_index;	//压栈右子树
			}
		}
		else
		{
			//b2 叶子节点： 遍历叶子节点中的三角形，寻找交点
			auto triangle_iter = ordered_triangles.begin() + node.triangle_index;  
			DEBUG_LINE(triangle_test_count += node.triangle_count;)
			for (size_t i = 0;i < node.triangle_count;i++)  //循环遍历这个节点的所有三角形
			{
				//检查迭代器对应的三角形与光线是否相交
				auto hit_info = triangle_iter->Intersect(ray, t_min, t_max);


				//迭代器往后移一位（指向下一个三角形）
				++triangle_iter;

				if (hit_info)
				{
					t_max = hit_info->t;
					closest_hit_info = hit_info;
					
					DEBUG_LINE(closest_hit_info->bounds_depth = node.depth;)	
				}
			}
			if (ptr == stack.begin()) break; //栈空：没有待办节点=>退出循环
			current_node_index = *(--ptr);   //栈不空：处理下一个待办节点=>下一轮循环
		}
	}
	if (closest_hit_info.has_value())
	{
		DEBUG_LINE(closest_hit_info->bounds_test_count=bounds_test_count;)
		DEBUG_LINE(closest_hit_info->triangle_test_count=triangle_test_count;)
	}

	return closest_hit_info;
}

/// @brief 将BVH树转化为BVH数组
/// @param node BVH树的一个节点
/// @return 这个节点经过DFS处理后在BVH数组中的位置
size_t BVH::RecursiveFlatten(BVHTreeNode* node)
{
	// 创建当前节点的线性节点
	BVHNode bvh_node{
		node->bounds,									// AABB
		0,												// Union(内部：右孩子索引/叶子：三角形索引)
		static_cast<uint16_t>(node->triangles.size()), 	// 三角形数量
		static_cast<uint8_t>(node->depth),				// 对应原先BVH树的深度
		static_cast<uint8_t>(node->split_axis)			//划分轴
	};
	

	// 记录当前节点在数组中的位置
	auto idx = nodes.size();               // 数组末尾 = 这个节点的位置
	nodes.push_back(bvh_node);             // 将这个BVH树节点加入BVH数组末尾

	// -- 如果当前节点在BVH树中不是叶子节点 --
	if (bvh_node.triangle_count==0)
	{
		// 左孩子 = node_idx + 1 (隐式)
		RecursiveFlatten(node->children[0]);

		// 右孩子 = node.child1_index (显式存储)
		nodes[idx].child1_index = RecursiveFlatten(node->children[1]);
	}
	else
	{
		// -- 如果当前节点在BVH树中是叶子节点 --
		nodes[idx].triangle_index = ordered_triangles.size(); //设置当前节点的三角形索引


		for (const auto& triangle : node->triangles)
		{

			//存放到全局三角形数组中
			ordered_triangles.push_back(triangle);
		}
	}

	// 返回这个节点经过处理后在BVH数组中的位置
	return idx;
}
