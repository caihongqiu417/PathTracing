#pragma once

#include "material.h"
#include "microfact_theory.h"

class ConductorMaterial :public Material
{
public:
	ConductorMaterial(const glm::vec3& ior, const glm::vec3& k,
	                  float alpha_x = 0.f, float alpha_z = 0.f)
		: ior(ior), k(k), microfact_theory(alpha_x, alpha_z) {}


	std::optional<BSDFSample> SampleBSDF(
		const glm::vec3& hit_point,
		const glm::vec3& view_direction,
		const Rng& rng) const override;

	glm::vec3 BSDF(const glm::vec3& hit_point, const glm::vec3& light_direction, const glm::vec3& view_direction) const override;

	// 粗糙导体：VNDF 重要性采样，p(ωi) = D_VNDF(wo, wh) / (4·dot(wo,wh))
	// Delta 导体：退化为 delta 分布，返回 1
	float PDF(const glm::vec3& hit_point, const glm::vec3& light_direction, const glm::vec3& view_direction) override;

    // Delta分布: 只有一个反射方向
	virtual bool IsDeltaDistribution()const override { return microfact_theory.IsDeltaDistribution(); }
private:
	// 导体的折射率用复数表示：ñ = ior + ik
	// ior（n）：复折射率实部，描述光速在介质中的减慢程度，与电介质的 IOR 含义相同
	// k：消光系数（extinction coefficient），复折射率虚部，描述光在介质内被吸收的速率
	//      k 越大，材质对光的吸收越强，颜色越深、表面越不透明
	// 两者都是 vec3，对 RGB 三通道分别取值，这样才能表现出金、铜等金属的彩色反射
	// 例：金  ior ≈ (0.17, 0.35, 1.5)  k ≈ (3.1, 2.4, 1.9)
	//     铜  ior ≈ (0.2,  0.9,  1.1)  k ≈ (3.9, 2.6, 2.4)
	glm::vec3 ior;  // 折射率实部 n
	glm::vec3 k;    // 消光系数（折射率虚部）
	MicrofactTheory microfact_theory;
};