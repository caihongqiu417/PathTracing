#include "conductor_material.h"

// 导体菲涅耳反射率（精确公式，参考 PBRT v3 §8.2）
// 输入 cos_theta_i：wo 与微表面法线夹角的余弦
// 返回 RGB 三通道反射率
static glm::vec3 Fresnel(const glm::vec3& ior, const glm::vec3& k, float cos_theta_i)
{
    float cos2  = glm::clamp(cos_theta_i, 0.f, 1.f);
    cos2        = cos2 * cos2;
    float sin2  = 1.f - cos2;

    glm::vec3 F;
    for (int i = 0; i < 3; ++i)
    {
        float n = ior[i], kv = k[i];
        float n2 = n * n, k2 = kv * kv;

        // 求复数平方根 √(n²-k²-sin²θ + 2nk·i) = a + b·i
        float t0   = n2 - k2 - sin2;
        float a2b2 = glm::sqrt(t0 * t0 + 4.f * n2 * k2);  // a² + b²
        float a    = glm::sqrt(0.5f * (a2b2 + t0));
        float b2   = 0.5f * (a2b2 - t0);                   // b²

        float cosTheta = glm::sqrt(cos2);

        // s 偏振
        float Rs = ((cosTheta - a) * (cosTheta - a) + b2) /
                   ((cosTheta + a) * (cosTheta + a) + b2);

        // p 偏振
        float a2b2_cos2 = a2b2 * cos2;
        float cross     = 2.f * a * cosTheta * sin2;
        float sin4      = sin2 * sin2;
        float Rp = Rs * (a2b2_cos2 - cross + sin4) /
                        (a2b2_cos2 + cross + sin4);

        // 非偏振光：两分量平均
        F[i] = 0.5f * (Rs + Rp);
    }
    return F;
}

// 粗糙导体 BRDF（用于直接光照，已知 ωi）
// f(ωi,ωo) = F(cos_wh) · D(wh) · G2(ωi,ωo,wh) / (4 · cosωi · cosωo)
// wh = normalize(ωi + ωo)（反射半向量）
// Delta 导体（光滑镜面）始终返回 0，因为任意给定的 ωi 几乎不可能命中精确反射方向
glm::vec3 ConductorMaterial::BSDF(const glm::vec3& hit_point,
    const glm::vec3& light_direction,
    const glm::vec3& view_direction) const
{
    if (microfact_theory.IsDeltaDistribution())
        return glm::vec3(0.f);

    // 两方向都须在上半球
    glm::vec3 wo = view_direction.y  > 0.f ? view_direction  : -view_direction;
    glm::vec3 wi = light_direction.y > 0.f ? light_direction : -light_direction;

    glm::vec3 wh = glm::normalize(wi + wo);
    if (wh.y <= 0.f) return glm::vec3(0.f);

    float cos_wh = glm::dot(wo, wh);
    if (cos_wh <= 0.f) return glm::vec3(0.f);

    float cos_wi = wi.y;
    float cos_wo = wo.y;
    if (cos_wi <= 0.f || cos_wo <= 0.f) return glm::vec3(0.f);

    glm::vec3 F = Fresnel(ior, k, cos_wh);
    float     D = microfact_theory.NormalDistribution(wh);
    float    G2 = microfact_theory.HeightCorrelatedMaskingShadowing(wi, wo, wh);

    return F * D * G2 / (4.f * cos_wi * cos_wo);
}

std::optional<BSDFSample> ConductorMaterial::SampleBSDF(
    const glm::vec3& hit_point,
    const glm::vec3& view_direction,
    const Rng& rng) const
{
    // 确保出射方向在上半球（局部坐标 y 轴 = 宏表面法线）
    glm::vec3 wo = view_direction.y > 0.f ? view_direction : -view_direction;

    // ---------- 采样微表面法线 ----------
    bool      is_delta = microfact_theory.IsDeltaDistribution();
    glm::vec3 wh       = is_delta
                         ? glm::vec3{0.f, 1.f, 0.f}
                         : microfact_theory.SampleVisibleNormal(wo, rng);

    float cos_wh = glm::dot(wo, wh);
    if (cos_wh <= 0.f) return std::nullopt;

    // ---------- 计算反射方向 ----------
    glm::vec3 wi = 2.f * cos_wh * wh - wo;
    if (wi.y <= 0.f) return std::nullopt;  // 反射到表面以下，无效

    // ---------- 菲涅耳 ----------
    glm::vec3 F     = Fresnel(ior, k, cos_wh);
    float     cos_i = wi.y;

    // ---------- 光滑导体（Delta BRDF）----------
    if (is_delta)
        return BSDFSample{F / cos_i, 1.f, wi};

    // ---------- 粗糙导体（微表面 BRDF）----------
    // VNDF 采样的权重化简为 F * G2 / G1(wo)
    float g1_wo = microfact_theory.Masking(wo, wh);
    float g2    = microfact_theory.HeightCorrelatedMaskingShadowing(wi, wo, wh);
    float g2_g1 = (g1_wo > 0.f) ? g2 / g1_wo : 0.f;

    return BSDFSample{F * g2_g1 / cos_i, 1.f, wi};
}

// Delta 导体（光滑镜面）：delta 分布，返回 1（配合 SampleBSDF 中 pdf=1 的权重使用）
// 粗糙导体（VNDF 采样）：
//   wh = normalize(ωi + ωo)（反射半向量）
//   p(wh) = D_VNDF(wo, wh) = D(wh) · G1(wo,wh) · dot(wo,wh) / cos(θo)
//   雅可比 |dwh/dωi| = 1 / (4·dot(wo,wh))
//   p(ωi) = D_VNDF(wo, wh) / (4·dot(wo,wh))
float ConductorMaterial::PDF(const glm::vec3& hit_point, const glm::vec3& light_direction, const glm::vec3& view_direction) const
{
    if (microfact_theory.IsDeltaDistribution()) return 0.f;

    glm::vec3 wo = view_direction.y  > 0.f ? view_direction  : -view_direction;
    glm::vec3 wi = light_direction.y > 0.f ? light_direction : -light_direction;

    glm::vec3 wh = glm::normalize(wi + wo);
    if (wh.y <= 0.f) return 0.f;

    float cos_wh = glm::dot(wo, wh);
    if (cos_wh <= 0.f) return 0.f;

    return microfact_theory.VisibleNormalDistribution(wo, wh) / (4.f * cos_wh);
}
