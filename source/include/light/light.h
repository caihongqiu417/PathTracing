#pragma once

#include"util/rng.h"
#include <glm/glm.hpp>
#include<optional>

// 光源采样结果：采样点、方向、辐射亮度，以及转换到立体角的 PDF
struct LightSample
{
	glm::vec3 light_point;     // 光源上的采样点（世界空间）
	glm::vec3 light_direction; // surface_point → light_point 的方向（已归一化）
	glm::vec3 Le;              // light_point 处朝向 surface_point 的辐射亮度
	float pdf;                 // 立体角 PDF = 面积 PDF × r² / cosθ
};

// 光源表面的辐射亮度
class Light
{
public:
	Light(const glm::vec3& Le)
		:Le(Le)
	{
	}

	// 获取光源功率
	virtual float Phi(float scene_radius)const;

	// 从光源表面重要性采样一个方向，返回 LightSample
	virtual std::optional<LightSample> SampleLight(const glm::vec3& surface_point, float scene_radius,
		const Rng& rng)const;

	// 返回光源在 light_point 处朝向 surface_point（被着色点）的辐射亮度
	virtual glm::vec3 GetRadiance(const glm::vec3& surface_point, const glm::vec3& light_point)const { return Le; }
protected:
	glm::vec3 Le; 
};

