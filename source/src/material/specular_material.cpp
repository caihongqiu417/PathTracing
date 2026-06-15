#include "specular_material.h"

std::optional<BSDFSample> SpecularMaterial::SampleBSDF(
    const glm::vec3& hit_point,
    const glm::vec3& view_direction,
    const Rng& rng) const
{
    // 完美镜面反射，方向由反射定律唯一确定（delta BRDF）
    glm::vec3 reflect_dir = glm::reflect(-view_direction, glm::vec3(0.f, 1.f, 0.f));

    float cos_theta = reflect_dir.y;  // 局部坐标下 y = 法线方向，即 cos(θ)

    // delta BRDF 的推导：
    //   渲染方程要求：beta *= f(wo,wi) * cos(θ) / pdf
    //   期望结果：   beta *= albedo（镜面反射保留 albedo 比例的能量）
    //   因此令：     f = albedo / cos(θ)，pdf = 1
    //   代入：       (albedo / cos) * cos / 1 = albedo ✓
    return BSDFSample{
        albedo / cos_theta,  // f = albedo / cos(θ)，除以 cos 是为了和渲染器的乘 cos 约掉
        1.0f,                // delta 分布视为 pdf = 1（只有一个采样点）
        reflect_dir
    };
}
