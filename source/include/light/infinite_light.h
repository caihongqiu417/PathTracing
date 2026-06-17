#pragma once

#include "light.h"

class InfiniteLight :public Light
{
public:
	InfiniteLight(const glm::vec3& Le)
		:Light(Le) {
	}

	// 获取光源功率
	virtual float Phi(float scene_radius)const override;

	// 从光源表面重要性采样一个方向，返回采样点
	virtual std::optional<LightSample> SampleLight(const glm::vec3& surface_point, float scene_radius,
													Rng& rng)const override;

	// 均匀球面采样：PDF = 1 / (4π)（与方向无关）
	virtual float GetPDF(const glm::vec3& surface_point, const glm::vec3& light_point, const glm::vec3& normal)const override;
};