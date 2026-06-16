#pragma once

#include "hit_info.h"
#include "ray.h"
#include"bounds.h"

#include <limits>
#include <optional>

// 面光源采样结果
struct ShapeSample
{
	glm::vec3 point;  // 光源表面上的采样点（世界坐标）
	glm::vec3 normal; // 该点的法线（世界坐标，朝外）
	float pdf;        // 面积测度下的采样概率密度（1/m²）；采样点用面积pdf，方向用立体角pdf
};

// 所有几何体的公共接口
class Shape
{
public:
	virtual ~Shape() = default;

	// 相交测试：强制子类实现
	virtual std::optional<HitInfo> Intersect(
		const Ray& ray,
		float t_min = 1e-5f,
		float t_max = (std::numeric_limits<float>::max)()
	) const = 0;

	// 获取自身包围盒
	virtual Bounds GetBounds()const { return {}; }

	// 获取自身表面积
	virtual float GetArea()const { return -1; }


	// 在几何体表面均匀采样一个点，用于直接光照（面光源采样）
	// 返回采样点的世界坐标、法线及对应 pdf；子类不实现则返回空（不可作面光源）
	virtual std::optional<ShapeSample> SampleShape(Rng& rng)const { return {}; }
};
