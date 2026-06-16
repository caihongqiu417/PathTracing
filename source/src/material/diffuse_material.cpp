#include "rng.h"
#include "sample/sphererical.h"
#include "diffuse_material.h"
#include <glm/gtc/constants.hpp>

// Lambertian BRDF：f(ωi, ωo) = albedo / π（与方向无关）
// 用于直接光照：已知光源方向 ωi，直接返回 f 值
glm::vec3 DiffuseMaterial::BSDF(const glm::vec3& hit_point,
    const glm::vec3& light_direction,
    const glm::vec3& view_direction) const
{
    // 入射或出射方向在表面以下，无贡献
    if (light_direction.y <= 0.f || view_direction.y <= 0.f)
        return glm::vec3(0.f);

    return albedo / glm::pi<float>();
}

std::optional<BSDFSample> DiffuseMaterial::SampleBSDF(
    const glm::vec3& hit_point,
    const glm::vec3& view_direction,
    const Rng& rng) const
{
    // 余弦重要性采样，方向分布与 Lambertian BRDF 的 cos 因子匹配
    glm::vec3 wi = CosineSampleHemisphere({ rng.Float01(), rng.Float01() });

    // Lambertian BRDF：f = albedo / π
    // 余弦重要性采样 PDF：p = cos(θ) / π
    // 渲染器执行 beta *= f * cos / pdf → (albedo/π) * cos / (cos/π) = albedo
    return BSDFSample{
        albedo / glm::pi<float>(),  // f = albedo / π
        wi.y / glm::pi<float>(),    // pdf = cos(θ) / π（wi.y = cos(θ) in local）
        wi
    };
}
