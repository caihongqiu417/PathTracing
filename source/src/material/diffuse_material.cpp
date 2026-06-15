
#include"rng.h"
#include"sample/sphererical.h"
#include "diffuse_material.h"

glm::vec3 DiffuseMaterial::SampleBSDF(const glm::vec3& hit_point,const glm::vec3& view_direction, glm::vec3& beta, const Rng& rng)const
{
	// 本轮：累加路径权重
	beta *= albedo;

	// 下一轮：生成反射光线
	return CosineSampleHemisphere({ rng.Float01(),rng.Float01() });
}