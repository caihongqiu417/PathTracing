#pragma once

#include "shape.h"
#include"scene_bvh.h"
#include"light/area_light.h"
#include"light/infinite_light.h"
#include"light/light_sampler.h"

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
		std::shared_ptr<Material> material=nullptr,
		const glm::vec3& pos = { 0, 0, 0 },
		const glm::vec3& scale = { 1, 1, 1 },
		const glm::vec3& rotate = { 0, 0, 0 }
	);

	// 添加面光源：将光源绑定到材质，注册几何体，并加入光源采样器
	void AddAreaLight(const AreaLight* area_light, std::shared_ptr<Material> material)
	{
		material->area_light = area_light;       
		AddShape(area_light->GetShape(), material); 
		light_sampler.AddLight(area_light);         
	}

	// 添加无限光源
	void AddInfiniteLight(const InfiniteLight* infinite_light)
	{
		light_sampler.AddLight(infinite_light);
	}

	// 获取采样器
	const LightSampler& GetLightSampler()const { return light_sampler; }

	//获取场景半径
	float GetRadius()const { return radius; }

	// 输入世界空间里的射线，返回世界空间里的最近交点
	std::optional<HitInfo> Intersect(
		const Ray& ray,
		float t_min = 1e-5f,
		float t_max = (std::numeric_limits<float>::max)()
	) const override;

	// 构建 BVH 并初始化光源采样器
	void Build()
	{
		scene_bvh.Build(std::move(instances));

		// 用包围盒对角线的一半作为场景半径
		Bounds bounds = scene_bvh.GetBounds();
		radius = glm::length(bounds.b_max - bounds.b_min) * 0.5f;
		light_sampler.Build(radius);
	}

private:
	std::vector<ShapeInstance> instances;
	SceneBVH scene_bvh{};

	LightSampler light_sampler;

	float radius;
};
