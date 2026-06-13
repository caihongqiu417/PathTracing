#pragma once

#include "hit_info.h"
#include "ray.h"
#include"bounds.h"

#include <limits>
#include <optional>

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
};
