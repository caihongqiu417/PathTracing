#include "area_light.h"
#include "sample/sphererical.h"

// 计算功率
float AreaLight::Phi(float scene_radius) const
{
	constexpr double PI = 3.141592653589793;

	// 取最大分量作为Phi
	return (double_side ? 2 : 1) * PI * shape.GetArea() * glm::max(Le.r, glm::max(Le.g, Le.b));
}

std::optional<LightSample> AreaLight::SampleLight(const glm::vec3& surface_point, float scene_radius, Rng& rng) const
{
	// 在面积上采样，需要雅可比转换dA=>dW
	auto shape_sample = shape.SampleShape(rng);
	if (!shape_sample.has_value())
	{
		return {};
	}

	// surface_point → light_point 的方向
	glm::vec3 light_direction_raw = shape_sample->point - surface_point; //r
	glm::vec3 light_direction = glm::normalize(light_direction_raw);

	// 面积 PDF 转立体角 PDF：p_ω = p_A × r² / |cosθ|
	// 即除以 Jacobian：det_J = |cosθ| / r²
	float cos_theta = glm::dot(shape_sample->normal, -light_direction); // cosθ 
	if (!double_side && cos_theta <= 0)
	{ // 如果是单面光源，且光照方向与法线夹角 >= 90°（背面），不发光
		return{};
	}
	float det_J = glm::abs(cos_theta) / glm::dot(light_direction_raw, light_direction_raw); // |cosθ| / r²

	return LightSample{shape_sample->point, light_direction, Le, shape_sample->pdf / det_J};
}


glm::vec3 AreaLight::GetRadiance(const glm::vec3& surface_point, const glm::vec3& light_point,const glm::vec3& normal) const
{
	// 如果是单面光源，且光照方向与法线夹角 >= 90°（背面），不发光
    if ((!double_side) && (glm::dot(surface_point - light_point, normal) <= 0))
    {
        return {};  
    }

}
