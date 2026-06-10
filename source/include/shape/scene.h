#pragma once

#include "shape.h"

#include <glm/glm.hpp>
#include <limits>
#include <vector>

// 坐标约定：
// 1. 相机、光源、射线默认都在世界空间里
// 2. 模型文件里的顶点先在模型空间里
// 3. ShapeInstance 负责在这两套坐标之间来回转换

struct ShapeInstance
{
	const Shape& shape;
	// 材质按值保存，避免 AddShape 传入临时对象时留下悬空引用。
	Material material;

	// 把模型空间变到世界空间
	glm::mat4 world_from_object;

	// 把世界空间变回模型空间
	glm::mat4 object_from_world;
};

struct Scene : public Shape
{
public:
	// 把一个物体放进场景，并设置它在世界空间里的位置、缩放和旋转、材质
	void AddShape(
		const Shape& shape,
		const Material& material = {},
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

private:
	std::vector<ShapeInstance> instances;
};
