#include "dielectric_material.h"
#include <cmath>

// 菲涅耳公式（精确版，适用于电介质）
// etai_div_etat : η_transmitted / η_incident（折射率之比）
// cos_theta_t   : 入射角余弦（已知量）
// cos_theta_i   : 折射角余弦（输出，由斯涅尔定律计算）
// 返回：反射率 [0,1]，全内反射时返回 1
static float Fresnel(float etai_div_etat, float cos_theta_t, float& cos_theta_i)
{
    float sin2_theta_t = 1.f - cos_theta_t * cos_theta_t;
    float sin2_theta_i = sin2_theta_t / (etai_div_etat * etai_div_etat);

    if (sin2_theta_i >= 1.f)
        return 1.f;  // 全内反射：折射角不存在

    cos_theta_i = glm::sqrt(1.f - sin2_theta_i);

    // Fresnel 方程：平行 / 垂直偏振分量的振幅反射率
    float r_parl = (etai_div_etat * cos_theta_t - cos_theta_i)
                 / (etai_div_etat * cos_theta_t + cos_theta_i);
    float r_perp = (cos_theta_t - etai_div_etat * cos_theta_i)
                 / (cos_theta_t + etai_div_etat * cos_theta_i);

    // 非偏振光：取两者的功率平均
    return (r_parl * r_parl + r_perp * r_perp) * 0.5f;
}

std::optional<BSDFSample> DielectricMaterial::SampleBSDF(
    const glm::vec3& hit_point,
    const glm::vec3& view_direction,
    const Rng& rng) const
{
    // view_direction.y > 0 → 光线从外部入射（空气 → 玻璃）
    // view_direction.y < 0 → 光线从内部射出（玻璃 → 空气）
    float cos_theta_t = view_direction.y;

    float     etai_div_etat = ior;       // 默认：η_trans / η_inc = ior / 1
    glm::vec3 normal        = {0, 1, 0};

    if (cos_theta_t < 0.f)
    {
        etai_div_etat = 1.f / ior;       // 玻璃 → 空气，比值倒置
        normal        = {0, -1, 0};      // 法线翻转（指向玻璃内侧）
        cos_theta_t   = -cos_theta_t;    // 取正值
    }

    float cos_theta_i = 0.f;             // 由 Fresnel 函数填写
    float reflectance = Fresnel(etai_div_etat, cos_theta_t, cos_theta_i);

    if (rng.Float01() <= reflectance)
    {
        // ── 反射分支（以概率 reflectance 选中）──
        // 概率 reflectance 与 Fresnel 权重相消，等价于直接使用 albedo_r
        glm::vec3 reflect_dir = {-view_direction.x, view_direction.y, -view_direction.z};

        // delta BRDF：f = albedo_r / cos(θ)，pdf = 1
        // 渲染器执行 f * cos / pdf = albedo_r
        return BSDFSample{
            albedo_r / reflect_dir.y,
            1.0f,
            reflect_dir
        };
    }
    else
    {
        // ── 折射分支（以概率 1 - reflectance 选中）──
        glm::vec3 refract_dir = (-view_direction / etai_div_etat)
                              + (glm::dot(normal, view_direction) / etai_div_etat - cos_theta_i) * normal;

        // 折射方向穿过表面，y 分量为负（朝法线对侧传播）
        // delta BTDF：f = albedo_t / |cos(θ)|，pdf = 1
        // 渲染器执行 f * |cos| / pdf = albedo_t
        float abs_cos = std::abs(refract_dir.y);
        return BSDFSample{
            albedo_t / abs_cos,
            1.0f,
            refract_dir
        };
    }
}
