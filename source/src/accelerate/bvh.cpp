#include "bvh.h"
#include<array>

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
	RecursiveSplit(root);
	
	//BVH树结构=>BVH线性结构
	RecursiveFlatten(root);
}



void BVH::RecursiveSplit(BVHTreeNode* node)
{
	//递归出口
	if (node->triangles.size() == 1)
		return;

	//找到包围盒最长的轴
	glm::vec3 diag = node->bounds.Diagonal();
	size_t max_axis;
	if (diag.x >= diag.y && diag.x >= diag.z)
    	max_axis = 0;  // X轴最长
	else if (diag.y >= diag.x && diag.y >= diag.z)
    	max_axis = 1;  // Y轴最长
	else
		max_axis = 2;  // Z轴最长

	// 计算最长轴上的中心点位置（起点 + 总长度的一半）
	float mid = node->bounds.b_min[max_axis] + diag[max_axis] * 0.5f;

	//划分到两个三角形列表中
	std::vector<Triangle> child0_triangles, child1_triangles;
	for (const auto& triangle : node->triangles)
	{
		// 计算三角形中心点在当前分割轴上的坐标
		float center = (triangle.p0[max_axis] + triangle.p1[max_axis] + triangle.p2[max_axis]) / 3.0f;

		// 判断中心点在分割轴的哪一侧
		if (center < mid)
		{
			child0_triangles.push_back(triangle);
		}
		else
		{
			child1_triangles.push_back(triangle);
		}
	}

	//创建两个子节点并分别放入三角形列表
	if (child0_triangles.empty() || child1_triangles.empty())
		return;

	auto* child0 = new BVHTreeNode();
	auto* child1 = new BVHTreeNode();
	node->children[0] = child0;
	node->children[1] = child1;

	node->triangles.clear();			//清空当前节点的三角形！
	node->triangles.shrink_to_fit();

	child0->triangles = std::move(child0_triangles);
	child1->triangles = std::move(child1_triangles);

	//子节点更新包围盒
	child0->UpdateBounds();
	child1->UpdateBounds();

	//递归分割子节点
	RecursiveSplit(child0);
	RecursiveSplit(child1);
}



//===========================================================================================
// 相交检测:遍历BVH结构，找到所有与光线相交的三角形
//===========================================================================================
std::optional<HitInfo> BVH::Intersect(const Ray& ray, float t_min, float t_max) const
{

	std::optional<HitInfo> closest_hit_info;


	//栈：代表待办节点
	std::array<int, 32> stack;
	auto ptr = stack.begin();

	//索引：代表当前遍历的节点
	size_t current_node_index = 0;

	while (true)
	{
		auto& node = nodes[current_node_index];


		// -- a.如果光线和当前节点没有交点 --
		if (!node.bounds.HasIntersection(ray, t_min, t_max))
		{

			if (ptr == stack.begin()) break; //a1 栈空：没有待办节点=>退出循环
			current_node_index = *(--ptr);   //a2 栈不空：处理下一个待办节点=>下一轮循环
			continue;
		}

		// -- b.如果光线和当前节点有交点 --
		if (node.triangle_count==0)
		{
			// b1 内部节点
			current_node_index++; 			//进左子树
			*(ptr++) = node.child1_index;	//压栈右子树
		}
		else
		{
			//b2 叶子节点： 遍历叶子节点中的三角形，寻找交点
			auto triangle_iter = ordered_triangles.begin() + node.triangle_index;  
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
				}
			}
			if (ptr == stack.begin()) break; //栈空：没有待办节点=>退出循环
			current_node_index = *(--ptr);   //栈不空：处理下一个待办节点=>下一轮循环
		}
	}

	return closest_hit_info;
}

/// @brief 将BVH树转化为BVH数组
/// @param node BVH树的一个节点
/// @return 这个节点经过DFS处理后在BVH数组中的位置
size_t BVH::RecursiveFlatten(BVHTreeNode* node)
{
	// 创建当前节点的线性节点
	BVHNode bvh_node
	{
		node->bounds,                  // 1. 包围盒：直接从树节点复制
		0,                             // 2. 右孩子索引：暂时为0，递归回溯时设置
		0,                             // 3. 三角形起始位置：指向全局三角形数组的位置
		node->triangles.size()         // 4. 三角形数量：用于遍历三角形范围
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
