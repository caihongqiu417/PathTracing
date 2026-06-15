#include "microfact_theory.h"
#include"util/rng.h"
#include"sample/sphererical.h"
#include"util/frame.h"

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
	// 微表面法线朝下（或在表面上），D(m) = 0
	if (microfact_normal.y <= 0.f)
		return 0.f;

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

// 可见法线分布函数
float MicrofactTheory::VisibleNormalDistribution(const glm::vec3& view_direction, const glm::vec3& microfact_normal) const
{
	glm::vec3 view_direction_upper = view_direction.y > 0 ? view_direction : -view_direction;

	//微表面背对观察者，不可见
	float cos_theta_o = glm::dot(view_direction_upper, microfact_normal); //观察方向与微表面法线的夹角
	if (cos_theta_o <= 0)
	{
		return 0;
	}
	return NormalDistribution(microfact_normal) * cos_theta_o * Masking(view_direction, microfact_normal) / glm::abs(view_direction.y);
}

// 采样可见法线
glm::vec3 MicrofactTheory::SampleVisibleNormal(const glm::vec3 &view_direction, const Rng &rng) const {
    // 1. 确保观察方向在上半球（微表面理论要求）
    glm::vec3 view_dir_up = view_direction.y > 0 ? view_direction : -view_direction;
    
    // 2. 将观察方向从各向异性空间变换到各向同性空间
    //    这样可以用标准GGX算法采样
    glm::vec3 view_dir_iso = glm::normalize(glm::vec3(
        alpha_x * view_dir_up.x,
        view_dir_up.y,
        alpha_z * view_dir_up.z
    ));
    
    // 3. 在单位圆盘上随机取一点（投影平面采样）
    float u1 = rng.Float01();
    float u2 = rng.Float01();
    glm::vec2 sample = UniformSampleUnitDisk({ u1, u2 });
    
    // 4. 计算圆盘边缘的高度
    float rim_height = glm::sqrt(1.f - sample.x * sample.x);
    
    // 5. 根据观察角度混合采样策略
    //    垂直观察时用圆盘内部采样，掠射时用边缘采样
    float mix = 0.5f * (1.f + view_dir_iso.y);
    sample.y = mix * sample.y + (1.f - mix) * rim_height;
    
    // 6. 构建以观察方向为Z轴的局部坐标系
    Frame local_frame(view_dir_iso);
    
    // 7. 将2D采样点提升到球面，得到各向同性空间的微表面法线
    //    y = sqrt(1 - x² - z²) 保证是单位向量
    glm::vec3 normal_iso = local_frame.WorldFromLocal({
        sample.x,
        glm::sqrt(1.f - sample.x * sample.x - sample.y * sample.y),
        sample.y
    });
    
    // 8. 变换回各向异性空间，得到最终法线
    return glm::normalize(glm::vec3(
        alpha_x * normal_iso.x,
        normal_iso.y,
        alpha_z * normal_iso.z
    ));
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
