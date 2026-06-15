#pragma once

#include <glm/glm.hpp>
#include "material.h"

// 射线打中物体后返回的信息
struct HitInfo
{
	float t;              // 交点离射线起点有多远
	glm::vec3 point;      // 交点，使用世界空间
	glm::vec3 normal;     // 交点处的法线，使用世界空间
	const Material* material = nullptr; //交点所对应物体的材质

	//调试信息
	#ifdef WITH_DEBUG_INFO
		size_t bounds_test_count    = 0;  // 检测了多少个包围盒
		size_t triangle_test_count  = 0;  // 检测了多少个三角形
		size_t instance_test_count  = 0;  // 检测了多少个场景实例（SceneBVH 用）
		size_t bounds_depth         = 0;  // 命中点所在叶子节点的深度
	#endif

	// 当找到一条光线与表面的交点时
	//HitInfo hit = bvh.Intersect(ray);

	// debug_info 记录的是：
	// 为了找到这个交点，BVH做了多少工作

	// hit.bounds_test_count    = 100    // 检测了100个包围盒
	// hit.triangle_test_count  = 50     // 检测了50个三角形
	// hit.bounds_depth         = 15     // 遍历了15层BVH树
};


