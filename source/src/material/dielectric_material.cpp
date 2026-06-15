#include "dielectric_material.h"
#include <cmath>

// 电介质菲涅耳反射率（精确公式）
// 输入 eta = η_transmitted / η_incident，cos_o：入射角余弦
// 输出 cos_t：折射角余弦（全内反射时置 0）
// 返回反射率 [0, 1]，全内反射时返回 1
static float Fresnel(float eta, float cos_o, float& cos_t)
{
    float sin2_o = 1.f - cos_o * cos_o;
    float sin2_t = sin2_o / (eta * eta);

    // 全内反射：折射角不存在
    if (sin2_t >= 1.f) { cos_t = 0.f; return 1.f; }

    cos_t = glm::sqrt(1.f - sin2_t);

    float r_parl = (eta * cos_o - cos_t) / (eta * cos_o + cos_t);
    float r_perp = (cos_o - eta * cos_t) / (cos_o + eta * cos_t);
    return (r_parl * r_parl + r_perp * r_perp) * 0.5f;
}

std::optional<BSDFSample> DielectricMaterial::SampleBSDF(
    const glm::vec3& hit_point,
    const glm::vec3& view_direction,
    const Rng& rng) const
{
    // 判断光线从哪一侧入射，决定折射率比值和表面朝向
    bool  entering = view_direction.y > 0.f;
    float eta      = entering ? ior : (1.f / ior);  // η_transmitted / η_incident

    // VNDF 采样需要 wo 在上半球（对准当前侧的表面法线方向）
    glm::vec3 wo_upper = entering ? view_direction : -view_direction;

    // ---------- 采样微表面法线 ----------
    bool      is_delta = microfact_theory.IsDeltaDistribution();
    glm::vec3 wh       = is_delta
                         ? glm::vec3{0.f, 1.f, 0.f}
                         : microfact_theory.SampleVisibleNormal(wo_upper, rng);

    float cos_o = glm::dot(wo_upper, wh);  // VNDF 保证大于 0
    if (cos_o <= 0.f) return std::nullopt;

    // ---------- 菲涅耳 ----------
    float cos_t = 0.f;
    float F     = Fresnel(eta, cos_o, cos_t);

    // VNDF 采样权重 G2/G1（光滑时退化为 1）
    auto g2_over_g1 = [&](const glm::vec3& wi_upper) -> float {
        if (is_delta) return 1.f;
        float g1 = microfact_theory.Masking(wo_upper, wh);
        float g2 = microfact_theory.HeightCorrelatedMaskingShadowing(wi_upper, wo_upper, wh);
        return (g1 > 0.f) ? g2 / g1 : 0.f;
    };

    // ---------- 反射分支 ----------
    // 以概率 F 选中，F 与菲涅耳权重相消，最终通量等比缩放 albedo_r
    if (rng.Float01() <= F)
    {
        glm::vec3 wi_upper = 2.f * cos_o * wh - wo_upper;
        if (wi_upper.y <= 0.f) return std::nullopt;  // 反射到表面以下，无效

        glm::vec3 wi    = entering ? wi_upper : -wi_upper;
        float     cos_i = glm::abs(wi.y);
        float     w     = g2_over_g1(wi_upper);
        return BSDFSample{albedo_r * w / cos_i, 1.f, wi};
    }

    // ---------- 折射分支 ----------
    // 以概率 1-F 选中，(1-F) 与菲涅耳权重相消，最终通量等比缩放 albedo_t
    glm::vec3 wi_upper = -(1.f / eta) * wo_upper + (cos_o / eta - cos_t) * wh;
    if (wi_upper.y >= 0.f) return std::nullopt;  // 折射方向应穿过表面（y < 0）

    glm::vec3 wi        = entering ? wi_upper : -wi_upper;
    float     cos_t_abs = glm::abs(wi.y);
    if (cos_t_abs == 0.f) return std::nullopt;

    float w = g2_over_g1(wi_upper);
    return BSDFSample{albedo_t * w / cos_t_abs, 1.f, wi};
}
