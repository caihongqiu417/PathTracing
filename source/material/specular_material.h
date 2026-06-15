#pragma once

#include "material.h"

class SpecularMaterial : public Material
{
public:
	SpecularMaterial(const glm::vec3& albedo)
		: albedo(albedo) {
	}

	virtual glm::vec3 SampleBSDF(const glm::vec3& hit_point, const glm::vec3& view_direction, glm::vec3& beta, const Rng& rng) const override;

private:
	glm::vec3 albedo{};
};
