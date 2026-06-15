#pragma once

#include <optional>
#include "util/rng.h"
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
    // 采样 BSDF
    // 参数：hit_point      交点位置（世界空间）
    //       view_direction  出射方向（局部坐标，指向观察者）
    //       rng             随机数生成器
    // 返回：BSDFSample，或 nullopt（如折射超过临界角）
    virtual std::optional<BSDFSample> SampleBSDF(
        const glm::vec3& hit_point,
        const glm::vec3& view_direction,
        const Rng& rng) const = 0;

    void SetEmissive(const glm::vec3& e) { emissive = e; }
    glm::vec3 emissive{};
};
