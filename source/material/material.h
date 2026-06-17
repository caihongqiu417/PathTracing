#pragma once

#include <optional>
#include "util/rng.h"
class AreaLight;
#include <glm/glm.hpp>

// =============================================================
// 一次 BSDF 采样的结果
//
// 渲染器拿到后执行：
//   beta *= bsdf * |cos(θ)| / pdf
//
// ── 漫反射（余弦重要性采样）──
//   bsdf = albedo / π            ← Lambertian BRDF
//   pdf  = cos(θ) / π            ← 余弦加权采样的概率密度
//   代入：(albedo/π) * cos / (cos/π) = albedo ✓
//
// ── delta 材质（镜面 / 导体 / 电介质）──
//   这类材质只有一个合法方向（反射方向 reflect_dir 或折射方向 refract_dir），
//   其 BRDF/BTDF 含 Dirac δ 函数。实现上令：
//     bsdf = reflectance / cos(θ)   reflectance 是该材质的实际能量衰减系数
//     pdf  = 1                       delta 分布离散化为单点，概率密度取 1
//   代入：(reflectance / cos) * cos / 1 = reflectance ✓
//   各材质的 reflectance：
//     镜面     → albedo（铝、金等材质的反照率）
//     电介质   → albedo_r（反射分支）或 albedo_t（折射分支）
//     导体     → fresnel_reflectance（由复折射率计算的菲涅耳反射率，RGB 各不同）
// =============================================================
struct BSDFSample
{
    glm::vec3 bsdf;             // BSDF 值 f(wo, wi)，单位 1/sr
    float     pdf;              // 采样方向的概率密度 p(wi)，单位 1/sr
    glm::vec3 light_direction;  // 采样到的入射方向（局部坐标，y 轴 = 法线）
};

class Material
{
public:
    // ── BSDF 采样（间接光 / 路径追踪）──────────────────────────────
    // 按材质分布重要性采样一个入射方向 ωi，返回 BSDFSample
    // 用途：路径追踪延伸下一段光线
    // 返回 nullopt 表示该方向无贡献（如折射全内反射）
    virtual std::optional<BSDFSample> SampleBSDF(
        const glm::vec3& hit_point,      // 交点位置（世界空间，用于纹理 / 切线空间）
        const glm::vec3& view_direction, // 出射方向 ωo（局部坐标，y 轴 = 法线，指向观察者）
        const Rng& rng) const = 0;

    // ── BSDF 评估（直接光照 / 光源采样）────────────────────────────
    // 给定已知入射方向 ωi（由光源采样得到），返回 f(ωi, ωo)
    // 用途：直接光照中计算光源方向的 BSDF 贡献
    // Delta 材质（镜面等）始终返回 0，因为任意指定的 ωi 几乎不可能等于精确反射方向
    virtual glm::vec3 BSDF(
        const glm::vec3& hit_point,      // 交点位置（局部坐标系构建用）
        const glm::vec3& light_direction,// 入射方向 ωi（表面 → 光源）
        const glm::vec3& view_direction  // 出射方向 ωo（表面 → 观察者）
    ) const = 0;

    // 返回概率密度
    virtual float PDF(const glm::vec3& hit_point, const glm::vec3& light_direction, const glm::vec3& view_direction);

    // Delta 分布：只有唯一一个合法入射方向（镜面 / 电介质 / 导体）
    // 积分器据此跳过直接光照的 BSDF 评估，只走 SampleBSDF 路径
    virtual bool IsDeltaDistribution() const = 0;

    // 该材质所属的面光源（nullptr 表示不发光）
    const AreaLight* area_light{ nullptr };
};
