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

// 粗糙电介质 BSDF（用于直接光照，已知 ωi）
// 根据 ωi 和 ωo 是否同侧，分反射和透射两种情况评估
// Delta 电介质（光滑玻璃）始终返回 0
glm::vec3 DielectricMaterial::BSDF(const glm::vec3& hit_point,
    const glm::vec3& light_direction,
    const glm::vec3& view_direction) const
{
    if (microfact_theory.IsDeltaDistribution())
        return glm::vec3(0.f);

    bool entering = view_direction.y > 0.f;
    float eta     = entering ? ior : (1.f / ior);  // η_t / η_i

    glm::vec3 wo_upper = entering ? view_direction : -view_direction;

    // ωi 与 ωo 同侧 → 反射；异侧 → 透射
    bool is_reflection = (light_direction.y > 0.f) == (view_direction.y > 0.f);

    if (is_reflection)
    {
        // 反射半向量
        glm::vec3 wi_upper = entering ? light_direction : -light_direction;
        if (wi_upper.y <= 0.f) return glm::vec3(0.f);

        glm::vec3 wh = glm::normalize(wi_upper + wo_upper);
        if (wh.y <= 0.f) return glm::vec3(0.f);

        float cos_o = glm::dot(wo_upper, wh);
        if (cos_o <= 0.f) return glm::vec3(0.f);

        float cos_t_unused;
        float F  = Fresnel(eta, cos_o, cos_t_unused);
        float D  = microfact_theory.NormalDistribution(wh);
        float G2 = microfact_theory.HeightCorrelatedMaskingShadowing(wi_upper, wo_upper, wh);

        float cos_wi = wi_upper.y;
        float cos_wo = wo_upper.y;
        if (cos_wi <= 0.f || cos_wo <= 0.f) return glm::vec3(0.f);

        // f_r = albedo_r · F · D · G2 / (4 · cosωi · cosωo)
        return albedo_r * F * D * G2 / (4.f * cos_wi * cos_wo);
    }
    else
    {
        // 透射方向在另一侧，翻转到上半球
        glm::vec3 wi_upper = entering ? -light_direction : light_direction;
        if (wi_upper.y <= 0.f) return glm::vec3(0.f);

        // 透射半向量：指向入射侧，wh = normalize(ωo + η·ωi_upper)
        glm::vec3 wh = glm::normalize(wo_upper + eta * wi_upper);
        if (wh.y <= 0.f) wh = -wh;  // 保证半向量在上半球

        float cos_o = glm::dot(wo_upper, wh);
        float cos_i = glm::dot(wi_upper, wh);
        if (cos_o <= 0.f || cos_i <= 0.f) return glm::vec3(0.f);

        float cos_t;
        float F  = Fresnel(eta, cos_o, cos_t);
        float D  = microfact_theory.NormalDistribution(wh);
        float G2 = microfact_theory.HeightCorrelatedMaskingShadowing(wi_upper, wo_upper, wh);

        float cos_wi = wi_upper.y;
        float cos_wo = wo_upper.y;
        if (cos_wi <= 0.f || cos_wo <= 0.f) return glm::vec3(0.f);

        // f_t = albedo_t · (1-F) · D · G2 · η² · cosωi_h · cosωo_h
        //       / (cosωi · cosωo · (cosωo_h + η·cosωi_h)²)
        float denom = cos_o + eta * cos_i;
        return albedo_t * (1.f - F) * D * G2 * eta * eta * cos_i * cos_o
               / (cos_wi * cos_wo * denom * denom);
    }
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
