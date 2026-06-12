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
	//三角形列表=>BVH树结构
	auto root = new BVHTreeNode{};
	root->triangles = std::move(triangles);
	root->UpdateBounds();

	//根节点深度设置为1
	root->depth = 1;

	BVHState state{};
	RecursiveSplit(root, state);

	std::cout << "[BVH] 总节点数:         " << state.total_node_count             << "\n"
	          << "[BVH] 叶子节点数:       " << state.leaf_node_count              << "\n"
	          << "[BVH] 叶子最大三角形数: " << state.max_leaf_node_triangle_count << "\n";

	//BVH树结构=>BVH线性结构
	RecursiveFlatten(root);
}



void BVH::RecursiveSplit(BVHTreeNode* node, BVHState& state)
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
	// 2. 选分割轴：包围盒最长的轴
	// -----------------------------------------------------------------------
	glm::vec3 diag = node->bounds.Diagonal();
	size_t max_axis;
	if      (diag.x >= diag.y && diag.x >= diag.z) max_axis = 0;
	else if (diag.y >= diag.x && diag.y >= diag.z) max_axis = 1;
	else                                            max_axis = 2;
	node->split_axis = max_axis;

	// -----------------------------------------------------------------------
	// 3. SAH：枚举 11 个分割点，选代价最小的
	//    代价 = SA(左) * N_左 + SA(右) * N_右，再除以父节点面积归一化
	//    SA 越大表示光线越容易击中该节点，N 越大表示需要测试的三角形越多
	// -----------------------------------------------------------------------
	float sa_parent = node->bounds.Area();
	float best_mid  = node->bounds.b_min[max_axis] + diag[max_axis] * 0.5f; // 默认退化为中点
	float min_cost  = std::numeric_limits<float>::infinity();

	for (size_t i = 0; i < 11; i++)
	{
		// 候选分割点：将最长轴均匀分成 12 份，取第 i+1 个刻度
		float mid = node->bounds.b_min[max_axis] + diag[max_axis] * (i + 1.f) / 12.f;

		// 遍历所有三角形，按重心坐标分到左右两侧，同时扩展各自的包围盒
		Bounds left_bounds, right_bounds;
		size_t left_count = 0, right_count = 0;
		bool left_init = false, right_init = false;

		for (const auto& triangle : node->triangles)
		{
			// 三角形重心在分割轴上的坐标
			float center = (triangle.p0[max_axis] + triangle.p1[max_axis] + triangle.p2[max_axis]) / 3.f;
			if (center < mid)
			{
				// 第一个三角形时用顶点初始化包围盒，避免原点污染
				if (!left_init) { left_bounds = Bounds(triangle.p0, triangle.p0); left_init = true; }
				left_bounds.Expand(triangle.p0);
				left_bounds.Expand(triangle.p1);
				left_bounds.Expand(triangle.p2);
				left_count++;
			}
			else
			{
				if (!right_init) { right_bounds = Bounds(triangle.p0, triangle.p0); right_init = true; }
				right_bounds.Expand(triangle.p0);
				right_bounds.Expand(triangle.p1);
				right_bounds.Expand(triangle.p2);
				right_count++;
			}
		}

		// 任一侧为空则为退化分割（所有三角形都在同一侧），跳过这个候选点
		if (left_count == 0 || right_count == 0) continue;

		// SAH 代价 = (SA_左 * N_左 + SA_右 * N_右) / SA_父
		// 除以父节点面积是为了归一化，使不同层级的代价可以比较
		float cost = (left_bounds.Area() * left_count + right_bounds.Area() * right_count) / sa_parent;
		if (cost < min_cost)
		{
			min_cost = cost;
			best_mid = mid; // 更新最优分割点
		}
	}

	// -----------------------------------------------------------------------
	// 4. 按最优分割点划分三角形
	// -----------------------------------------------------------------------
	std::vector<Triangle> child0_triangles, child1_triangles;
	for (const auto& triangle : node->triangles)
	{
		float center = (triangle.p0[max_axis] + triangle.p1[max_axis] + triangle.p2[max_axis]) / 3.f;
		if (center < best_mid)
			child0_triangles.push_back(triangle);
		else
			child1_triangles.push_back(triangle);
	}

	// 极端聚集情况：所有三角形仍落在同一侧，无法继续分割
	if (child0_triangles.empty() || child1_triangles.empty())
	{
		state.AddLeafNode(node);
		return;
	}

	// -----------------------------------------------------------------------
	// 5. 创建子节点并递归
	// -----------------------------------------------------------------------
	auto* child0 = new BVHTreeNode();
	auto* child1 = new BVHTreeNode();
	node->children[0] = child0;
	node->children[1] = child1;

	node->triangles.clear();        // 内部节点不存三角形
	node->triangles.shrink_to_fit();

	child0->depth = node->depth + 1;
	child1->depth = node->depth + 1;
	child0->triangles = std::move(child0_triangles);
	child1->triangles = std::move(child1_triangles);
	child0->UpdateBounds();
	child1->UpdateBounds();

	RecursiveSplit(child0, state);
	RecursiveSplit(child1, state);
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
