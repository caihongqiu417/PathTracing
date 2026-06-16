#include "infinite_light.h"
#include "sample/sphererical.h"
#include <glm/gtc/constants.hpp>

// 无限远球形光源的总功率：Φ = 4π r² · Le
// 均匀覆盖整个场景包围球表面，面积 = 4π r²
float InfiniteLight::Phi(float scene_radius) const
{
    return 4.f * glm::pi<float>() * scene_radius * scene_radius * glm::max(Le.r, glm::max(Le.g, Le.b));
}

// 均匀采样球面上一个方向，light_point 放在包围球表面
// 均匀球面采样的立体角 PDF = 1 / (4π)
std::optional<LightSample> InfiniteLight::SampleLight(const glm::vec3& surface_point, float scene_radius, Rng& rng) const
{
    // 均匀采样整个球面，得到入射方向
    glm::vec3 direction = UniformSampleSphere(rng);

    // 光源点放在包围球表面（沿该方向的远端）
    glm::vec3 light_point = surface_point + direction * (2.f * scene_radius);

    // 均匀球面采样：pdf = 1 / (4π)
    float pdf = 1.f / (4.f * glm::pi<float>());

    return LightSample{ light_point, direction, Le, pdf };
}
