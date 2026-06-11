#pragma once

#include "renderer/base_renderer.h"

/// @brief 显示BVH包围盒检测次数的热力图
/// 蓝色表示检测次数少，红色表示检测次数多
/// 用于可视化BVH加速结构的效率
DEFINE_RENDERER(BoundsTest, "BVH Bounds Test Heatmap")

/// @brief 显示三角形检测次数的热力图
/// 蓝色表示检测次数少，红色表示检测次数多
/// 用于找出需要更多细分的区域
DEFINE_RENDERER(TriangleTest, "Triangle Test Heatmap")

/// @brief 显示BVH树遍历深度的热力图
/// 蓝色表示树深度浅，红色表示树深度深
/// 用于检查BVH树的平衡性
DEFINE_RENDERER(BoundsDepth, "BVH Tree Depth Heatmap")
