
#include "specular_material.h"

glm::vec3 SpecularMaterial::SampleBSDF(const glm::vec3& hit_point,const glm::vec3& view_direction, glm::vec3& beta, const Rng& rng) const
{
	// 本轮：累乘镜面反射率
	beta *= albedo;

	// 下一轮：在 local space 中，法线为 (0,1,0)，完美镜面反射
	return glm::reflect(-view_direction, glm::vec3(0.f, 1.f, 0.f));
}
