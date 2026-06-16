#pragma once

#include "material.h"

class SpecularMaterial : public Material
{
public:
	SpecularMaterial(const glm::vec3& albedo)
		: albedo(albedo) {
	}

	std::optional<BSDFSample> SampleBSDF(
		const glm::vec3& hit_point,
		const glm::vec3& view_direction,
		const Rng& rng) const override;

	// Delta 分布：任意给定 ωi 的概率为 0，始终返回 0
	glm::vec3 BSDF(const glm::vec3& hit_point, const glm::vec3& light_direction, const glm::vec3& view_direction) const override
	{
		return glm::vec3(0.f);
	}


    // Delta分布: 只有一个反射方向
	virtual bool IsDeltaDistribution()const override { return true; }
	
private:
	glm::vec3 albedo{};
};
