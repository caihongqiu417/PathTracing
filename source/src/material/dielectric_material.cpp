#include "dielectric_material.h"

/// @brief 计算 Fresnel 反射率
/// @param etai_div_etat 折射率之比 η_t / η_i（折射介质 / 入射介质）
/// @param cos_theta_t 入射角的余弦值（view_direction 与法线的点积）
/// @param cos_theta_i 折射角的余弦值，由函数内部通过斯涅尔定律计算后写出
/// @return 反射率 [0, 1]，1 表示全反射
float Fresnel(float etai_div_etat, float cos_theta_t, float& cos_theta_i)
{
	// i= incident（入射） t = transmitted（透射）
	float sin2_theta_t = 1 - cos_theta_t * cos_theta_t;
	float sin2_theta_i = sin2_theta_t / (etai_div_etat * etai_div_etat);

	//发生全反射
	if (sin2_theta_i >= 1)
		return 1;

	//算出折射角的 cos
	cos_theta_i = glm::sqrt(1 - sin2_theta_i);

	// r_parl：平行偏振分量   r_perp：垂直偏振分量
	float r_parl = (etai_div_etat * cos_theta_t - cos_theta_i)
	             / (etai_div_etat * cos_theta_t + cos_theta_i);
	float r_perp = (cos_theta_t - etai_div_etat * cos_theta_i)
	             / (cos_theta_t + etai_div_etat * cos_theta_i);

	// 返回反射率（决定走反射还是折射）
	return (r_parl * r_parl + r_perp * r_perp) / 2.f;
}

glm::vec3 DielectricMaterial::SampleBSDF(const glm::vec3& hit_point, const glm::vec3& view_direction, glm::vec3& beta, const Rng& rng) const
{
	
	//根据观察方向y分量的正负，判断光线是从外面射入还是从里面射出
	float cos_theta_t = view_direction.y;


	// -- a. 光线从外面射入 --
	float etai_div_etat = ior / 1.f; /* η1/η2 */
	glm::vec3 normal = { 0,1,0 };


	// -- b. 光线从里面射出 --
	if (cos_theta_t < 0)
	{

		etai_div_etat = 1.f / ior;   	//介质射入空气

		normal = { 0,-1,0 };    		//法线翻转
		cos_theta_t = -cos_theta_t;		//入射光翻转
	}

	// 计算得到透射角和反射率
	float cos_theta_i;
	float fr = Fresnel(etai_div_etat, cos_theta_t, cos_theta_i);

	if (rng.Float01() <= fr)
	{
		// -- a.反射 --
		beta *= albedo_r;
		return glm::vec3{ -view_direction.x,view_direction.y,-view_direction.z };
	}
	else
	{
		// -- b.透射 --	
		beta *= albedo_t;
		return glm::vec3{ (-view_direction / etai_div_etat)
				+ (glm::dot(normal,view_direction) / etai_div_etat - cos_theta_i) * normal }; //透射光方向
	}
}