#include "renderer/rt_renderer.h"
#include "util/frame.h"
#include <cmath>

glm::vec3 RTRenderer::RenderPixel(const glm::ivec2& pixel_coord)
{
    constexpr int   max_bounces    = 16;
    constexpr float ray_origin_bias = 1e-4f;

    glm::vec3 beta  = {1, 1, 1};
    glm::vec3 color = {0, 0, 0};

    Ray ray = camera.GenerateRay(pixel_coord, rng.PixelOffset());

    for (int bounce = 0; bounce < max_bounces; ++bounce)
    {
        auto hit_info = scene.Intersect(ray);
        if (!hit_info.has_value()) break;

        color += beta * hit_info->material->emissive;

        Frame     frame(hit_info->normal);
        glm::vec3 view_local = frame.LocalFromWorld(-ray.direction);

        auto sample_opt = hit_info->material->SampleBSDF(
            hit_info->point, view_local, rng);

        if (!sample_opt.has_value()) break;

        const auto& s = sample_opt.value();

        // beta *= f(wo,wi) * |cos(θi)| / pdf(wi)
        float cos_theta = std::abs(s.light_direction.y);
        beta *= s.bsdf * cos_theta / s.pdf;

        glm::vec3 new_dir_world = frame.WorldFromLocal(s.light_direction);
        ray.origin    = hit_info->point + new_dir_world * ray_origin_bias;
        ray.direction = new_dir_world;
    }

    return color;
}
