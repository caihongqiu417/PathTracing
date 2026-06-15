#pragma once

#include "shape.h"
#include"scene_bvh.h"

#include <glm/glm.hpp>
#include <limits>
#include <memory>
#include <vector>

// ShapeInstance 定义在 scene_bvh.h 中（已通过上面的 include 引入）

struct Scene : public Shape
{
public:
	// 把一个物体放进场景，并设置它在世界空间里的位置、缩放和旋转、材质
	void AddShape(
		const Shape& shape,
		std::shared_ptr<Material> material,
		const glm::vec3& pos = { 0, 0, 0 },
		const glm::vec3& scale = { 1, 1, 1 },
		const glm::vec3& rotate = { 0, 0, 0 }
	);

	// 输入世界空间里的射线，返回世界空间里的最近交点
	std::optional<HitInfo> Intersect(
		const Ray& ray,
		float t_min = 1e-5f,
		float t_max = (std::numeric_limits<float>::max)()
	) const override;

	//构建场景级包围盒
	void Build() { scene_bvh.Build(std::move(instances)); }

private:
	std::vector<ShapeInstance> instances;
	SceneBVH scene_bvh{};
};
