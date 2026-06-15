#include "conductor_material.h"
#include "complex_number.h"

glm::vec3 ConductorMaterial::SampleBSDF(const glm::vec3& hit_point, const glm::vec3& view_direction, glm::vec3& beta, const Rng& rng) const
{
	float cos_theta = view_direction.y;  

	glm::vec3 fr{};

	// 导体的 IOR 是复数（n + ik），RGB 三通道各不相同，分别计算菲涅耳反射率
	for (size_t i = 0; i < 3; i++)
	{
		Complex eta{ ior[i], k[i] }; 

		// sin²θ_t = sin²θ_i / η²
		float sin2_theta_i = 1.f - cos_theta * cos_theta;
		Complex sin2_theta_t = sin2_theta_i / (eta * eta);

		Complex cos_theta_t = sqrt(Complex{ 1.f } - sin2_theta_t);

		Complex cos_c{ cos_theta };  

		// r_s = (cos_θ_i - η·cos_θ_t) / (cos_θ_i + η·cos_θ_t)
		Complex r_perp = (cos_c - eta * cos_theta_t) / (cos_c + eta * cos_theta_t);

		// r_p = (η·cos_θ_i - cos_θ_t) / (η·cos_θ_i + cos_θ_t)
		Complex r_parl = (eta * cos_c - cos_theta_t) / (eta * cos_c + cos_theta_t);

		// 非偏振光的反射率 = (|r_s|² + |r_p|²) / 2
		fr[i] = (norm(r_perp) + norm(r_parl)) * 0.5f;
	}

	// 更新路径权重
	beta *= fr;

	// 导体只有镜面反射，无透射，方向唯一确定
	return { -view_direction.x, view_direction.y, -view_direction.z };
}