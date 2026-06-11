#include "bvh.h"

BVH::~BVH()
{
	DeleteNode(root);
}

void BVH::DeleteNode(BVHNode* node)
{
	if (!node)
		return;

	DeleteNode(node->children[0]);
	DeleteNode(node->children[1]);
	delete node;
}

//===========================================================================================
// 构建BVH
// 功能：将输入的三角形列表组织成BVH树结构，加速光线追踪查询
// 流程：1. 创建根节点并接管所有三角形
//       2. 计算根节点的包围盒
//       3. 递归分割节点，直到每个叶子节点包含少量三角形
// 注意：分割后，只有叶子节点才存储三角形，内部节点三角形列表会清空
//===========================================================================================
void BVH::Build(std::vector<Triangle>&& triangles)
{
	
	root = new BVHNode{};	//创建根节点

	root->triangles = std::move(triangles); //根节点接管三角形

	root->UpdateBounds();  //根节点扩展包围盒

	RecursiveSplit(root); //递归分割
}


void BVH::RecursiveSplit(BVHNode* node)
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

	auto* child0 = new BVHNode();
	auto* child1 = new BVHNode();
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
// 相交检测
//===========================================================================================
std::optional<HitInfo> BVH::Intersect(const Ray& ray, float t_min, float t_max) const
{
	//创建 closest_hit_info
	std::optional<HitInfo> closest_hit_info;

	//递归相交检测并填充closest_hit_info
	RecursiveIntersect(root, ray, t_min, t_max, closest_hit_info);

	return closest_hit_info;
}

void BVH::RecursiveIntersect(BVHNode* node, const Ray& ray, float t_min, float& t_max, std::optional<HitInfo>& closest_hit_info) const
{
	//判断和当前节点的包围盒是否有交点
	if (!node->bounds.HasIntersection(ray, t_min, t_max))
		return;

	//如果有交点
	if (node->triangles.empty()	/*不是叶子节点*/)
	{
		//递归向下找
		RecursiveIntersect(node->children[0], ray, t_min, t_max, closest_hit_info);
		RecursiveIntersect(node->children[1], ray, t_min, t_max, closest_hit_info);
	}
	else  						/*是叶子节点*/
	{
		//返回与叶子节点三角形相交的结果
		for (const auto& triangle : node->triangles)
		{
			auto hit_info = triangle.Intersect(ray, t_min, t_max);
			if (hit_info.has_value())
			{
				t_max = hit_info->t;
				closest_hit_info = hit_info;
			}
		}
	}
}
