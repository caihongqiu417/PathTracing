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

private:
	glm::vec3 albedo{};
};