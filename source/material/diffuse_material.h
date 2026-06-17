#pragma once

#include "material.h"

class DiffuseMaterial :public Material
{
public:
	DiffuseMaterial(const glm::vec3& albedo)
		: albedo(albedo) {
	}

	std::optional<BSDFSample> SampleBSDF(
		const glm::vec3& hit_point,
		const glm::vec3& view_direction,
		const Rng& rng) const override;

	glm::vec3 BSDF(const glm::vec3& hit_point, const glm::vec3& light_direction, const glm::vec3& view_direction) const override;

	// 余弦重要性采样：p(ωi) = cos(θi) / π
	float PDF(const glm::vec3& hit_point, const glm::vec3& light_direction, const glm::vec3& view_direction) override;

    // Delta分布: 只有一个反射方向
	virtual bool IsDeltaDistribution()const override { return false; }
private:
	glm::vec3 albedo{};
};