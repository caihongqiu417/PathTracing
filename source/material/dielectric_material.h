#pragma once

#include "material.h"

//=================================================================
// 电介质材质: 不导电的透明或半透明物质，光打到上面会同时发生反射和折射
//=================================================================
class DielectricMaterial :public Material
{
public:
	DielectricMaterial(float ior, const glm::vec3& albedo)
		:ior(ior),albedo_r(albedo),albedo_t(albedo)
	{
		
	}

	DielectricMaterial(float ior, const glm::vec3& albedo_r, const glm::vec3& albedo_t)
		:ior(ior),albedo_r(albedo_r),albedo_t(albedo_t)
	{
		
	}

	std::optional<BSDFSample> SampleBSDF(
		const glm::vec3& hit_point,
		const glm::vec3& view_direction,
		const Rng& rng) const override;

private:
	//折射率
	float ior;

	//反射时的反照率
	glm::vec3 albedo_r;

	//透射时的反照率
	glm::vec3 albedo_t;
};