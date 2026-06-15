#include "conductor_material.h"
#include "complex_number.h"

std::optional<BSDFSample> ConductorMaterial::SampleBSDF(
    const glm::vec3& hit_point,
    const glm::vec3& view_direction,
    const Rng& rng) const
{
    float cos_theta = view_direction.y;  // 局部坐标下 y 轴 = 法线方向

    // ── 计算菲涅耳反射率（RGB 三通道独立）──
    // 导体折射率为复数 η = n + ik，每通道 n、k 不同，产生彩色金属光泽
    glm::vec3 fresnel_reflectance{};
    for (size_t i = 0; i < 3; i++)
    {
        Complex eta{ ior[i], k[i] };  // 该通道的复折射率

        // 斯涅尔定律推广到复数域：sin²θ_out = sin²θ_in / η²
        float   sin2_theta_in  = 1.f - cos_theta * cos_theta;
        Complex sin2_theta_out = sin2_theta_in / (eta * eta);
        Complex cos_theta_out  = sqrt(Complex{1.f} - sin2_theta_out);

        Complex cos_in{ cos_theta };  // 入射角余弦包装为复数（虚部 = 0）

        // Fresnel 公式：垂直 / 平行偏振分量
        Complex r_perp = (cos_in - eta * cos_theta_out) / (cos_in + eta * cos_theta_out);
        Complex r_parl = (eta * cos_in - cos_theta_out) / (eta * cos_in + cos_theta_out);

        // 非偏振光反射率 = (|r_s|² + |r_p|²) / 2
        fresnel_reflectance[i] = (norm(r_perp) + norm(r_parl)) * 0.5f;
    }

    // ── 导体只有镜面反射（delta BRDF），方向唯一确定 ──
    glm::vec3 reflect_dir = {-view_direction.x, view_direction.y, -view_direction.z};

    float cos_reflect = reflect_dir.y;  // = cos_theta（反射不改变与法线夹角）

    // delta BRDF：f = fresnel_reflectance / cos(θ)，pdf = 1
    // 渲染器执行 f * cos / pdf = fresnel_reflectance（菲涅耳反射率即为每弹射的能量比）
    return BSDFSample{
        fresnel_reflectance / cos_reflect,
        1.0f,
        reflect_dir
    };
}
