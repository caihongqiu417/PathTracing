#pragma once

#include "material.h"
#include "microfact_theory.h"

//=================================================================
// 电介质材质: 不导电的透明或半透明物质，光打到上面会同时发生反射和折射
//=================================================================
class DielectricMaterial :public Material
{
public:
	DielectricMaterial(float ior, const glm::vec3& albedo,float alpha_x=0,float alpha_z=0)
		:ior(ior), albedo_r(albedo), albedo_t(albedo), microfact_theory(alpha_x, alpha_z)
	{
		
	}

	DielectricMaterial(float ior, const glm::vec3& albedo_r, const glm::vec3& albedo_t,float alpha_x=0,float alpha_z=0)
		:ior(ior),albedo_r(albedo_r),albedo_t(albedo_t),microfact_theory(alpha_x, alpha_z)
	{
		
	}

	std::optional<BSDFSample> SampleBSDF(
		const glm::vec3& hit_point,
		const glm::vec3& view_direction,
		const Rng& rng) const override;

	glm::vec3 BSDF(const glm::vec3& hit_point, const glm::vec3& light_direction, const glm::vec3& view_direction) const override;

	// 反射（ωi、ωo 同侧）：p(ωi) = F · D_VNDF(wo, wh) / (4·dot(wo,wh))
	// 折射（ωi、ωo 异侧）：p(ωi) = (1-F) · D_VNDF(wo, wh) · η²·|dot(wi,wh)| / (dot(wo,wh) + η·|dot(wi,wh)|)²
	// Delta 电介质：返回 1
	float PDF(const glm::vec3& hit_point, const glm::vec3& light_direction, const glm::vec3& view_direction) const override;

    // Delta分布: 只有一个反射方向
	virtual bool IsDeltaDistribution()const override { return microfact_theory.IsDeltaDistribution(); }

private:
	//折射率
	float ior;

	//反射时的反照率
	glm::vec3 albedo_r;

	//透射时的反照率
	glm::vec3 albedo_t;

	//微表面
	MicrofactTheory microfact_theory; //包括x轴和z轴方向的粗糙度
};