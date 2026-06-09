#pragma once

#include "hit_info.h"
#include "ray.h"

#include <limits>
#include <optional>

// =============================================================================
// Shape — 几何体抽象基类
// =============================================================================
class Shape
{
public:
	virtual ~Shape() = default;

	virtual std::optional<HitInfo> Intersect(
		const Ray& ray,
		float t_min = 1e-5f,
		float t_max = std::numeric_limits<float>::infinity()
	) const = 0;
};
