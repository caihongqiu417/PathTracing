#include "light.h"

float Light::Phi(float scene_radius) const
{
    return 0.f;
}

std::optional<LightSample> Light::SampleLight(const glm::vec3& surface_point, float scene_radius, Rng& rng) const
{
    return std::nullopt;
}
