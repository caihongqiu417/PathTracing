#include "microfact_theory.h"
static constexpr float PI = 3.14159265358979323846f;
// -- 传入粗糙度 --
MicrofactTheory::MicrofactTheory(float alpha_x, float alpha_z)
{
	// 确保粗糙度≠0
	this->alpha_x = glm::clamp(alpha_x * alpha_x, 1e-3f, 1.f);
	this->alpha_z=glm::clamp(alpha_z * alpha_z, 1e-3f, 1.f);
}

// -- 法线分布函数 --
float MicrofactTheory::NormalDistribution(const glm::vec3& microfact_normal) const
{
	//根据法线计算微面斜率
	glm::vec2 slope{ -microfact_normal.x / microfact_normal.y,-microfact_normal.z / microfact_normal.y };

	// 伸缩不变性：各向异性 GGX = 各向同性 GGX 在斜率空间拉伸 (αx, αz) 倍后的结果
	// 将拉伸后的斜率除以对应粗糙度，还原到各向同性空间，再用各向同性公式计算
	slope.x /= alpha_x;
	slope.y /= alpha_z;

	//P22_各向异性(sx, sz)  =  P22_各向同性(sx/αx, sz/αz) / (αx · αz)
	float slope_distribution = SlopeDistribution(slope) / (alpha_x * alpha_z);

	//将斜率分布函数转换为法线分布函数
	return slope_distribution / glm::pow(microfact_normal.y, 4);
}

// 遮蔽函数
float MicrofactTheory::Masking(const glm::vec3& view_direction, const glm::vec3& mirofact_normal) const
{
	// Lambda 只对上半球有定义，翻转保证 y > 0
	glm::vec3 view_direction_upper = view_direction.y > 0 ? view_direction : -view_direction;

	// dot(wo, wh) <= 0：观察者在该微表面的背面
	if (glm::dot(view_direction_upper, mirofact_normal) <= 0)
		return 0;

	// Smith G1 = 1 / (1 + Λ(w))
	// Λ 越大（粗糙度越高 / 掠射角越大）→ 遮蔽越严重 → G1 越小
	return 1.f / (1.f + Lambda(view_direction_upper));
}

// 遮蔽-阴影函数
float MicrofactTheory::HeightCorrelatedMaskingShadowing(const glm::vec3& light_direction, const glm::vec3& view_direction, const glm::vec3& microfact_normal)const
{
    // 翻转到上半球，Lambda 只对 y > 0 有定义
    glm::vec3 wi_upper = light_direction.y > 0 ? light_direction : -light_direction;
    glm::vec3 wo_upper = view_direction.y  > 0 ? view_direction  : -view_direction;

    // 任意一侧在该微表面的背面，则该微表面对 BRDF 无贡献
    if (glm::dot(wi_upper, microfact_normal) <= 0 ||
        glm::dot(wo_upper, microfact_normal) <= 0)
        return 0.f;

    // G2 = 1 / (1 + Λ(wi) + Λ(wo))
    return 1.f / (1.f + Lambda(wi_upper) + Lambda(wo_upper));
}

bool MicrofactTheory::IsDeltaDistribution() const
{
	return glm::max(alpha_x, alpha_z) == 1e-3f;
}

// 拉伸前的斜率分布函数
float MicrofactTheory::SlopeDistribution(const glm::vec2& slope) const
{
	return 1.f / (PI * glm::pow(1.f + slope.x * slope.x + slope.y * slope.y, 2));
}

// GGX 的 Λ(w) = (-1 + √(1 + (αx·wx/wy)² + (αz·wz/wy)²)) / 2
// 含义：从 direction_upper 方向看去，被遮蔽面积与可见面积之比
//       Λ = 0 → 完全可见；Λ → ∞ → 完全遮蔽
float MicrofactTheory::Lambda(const glm::vec3& direction_upper) const {
	if (direction_upper.y == 0)
		return std::numeric_limits<float>::infinity();
	float length2 = direction_upper.x * direction_upper.x + direction_upper.z * direction_upper.z;
	if (length2 == 0)
		return 0;
	float cos2_phi = direction_upper.x * direction_upper.x / length2;
    float sin2_phi = direction_upper.z * direction_upper.z / length2;
    float tan2_theta = length2 / (direction_upper.y * direction_upper.y);
    float alpha0_2 = alpha_x * alpha_x * cos2_phi + alpha_z * alpha_z * sin2_phi;
    return 0.5f * (glm::sqrt(1.f + alpha0_2 * tan2_theta) - 1.f);
}
