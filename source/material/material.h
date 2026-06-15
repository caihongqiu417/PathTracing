#pragma once

#include"util/rng.h"
#include<glm/glm.hpp>

class Material
{
public:
	/// @brief 采样 BSDF，返回新的光线方向，并更新路径权重
	/// @param view_direction 入射方向（指向相机/观察者，单位向量）
	/// @param beta 路径权重，函数内按 BRDF/pdf 累乘更新
	/// @param rng 随机数生成器，用于重要性采样
	/// @return 出射方向（单位向量），即下一段光线的传播方向
	// note:BSDF包括——BRDF — 反射（出射和入射在法线同侧）
	//				BTDF — 透射（出射穿过表面到另一侧，如玻璃、水）
	virtual glm::vec3 SampleBSDF(const glm::vec3& hit_point,const glm::vec3& view_direction, glm::vec3& beta, const Rng& rng) const = 0;




	void SetEmissive(const glm::vec3& e) { emissive = e; }

	glm::vec3 emissive{};
};