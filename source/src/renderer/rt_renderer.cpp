#include "renderer/rt_renderer.h"
#include "util/frame.h"

glm::vec3 RTRenderer::RenderPixel(const glm::ivec2& pixel_coord)
{
    //==============================
    // 初始化
    //==============================
    constexpr int max_bounces = 16;          // 最大反弹次数
    constexpr float ray_origin_bias = 1e-4f;// 避免自相交的偏移量

    glm::vec3 beta = { 1.f, 1.f, 1.f };     // 路径衰减系数
    glm::vec3 color = { 0.f, 0.f, 0.f };    // 累积颜色

    // 生成主光线（带抖动抗锯齿）
    Ray ray = camera.GenerateRay(pixel_coord, rng.PixelOffset());

    //==============================
    // 路径追踪
    //==============================
    for (int bounce = 0; bounce < max_bounces; ++bounce)
    {
        // 1. 求交
        auto hit_info = scene.Intersect(ray);
        if (!hit_info.has_value()) break;

        // 2. 累积贡献
        color += beta * hit_info->material->emissive;   // 加自发光

        // 3. 生成新光线（材质决定方向和 beta 更新）
        Frame frame(hit_info->normal);
        glm::vec3 view_local = frame.LocalFromWorld(-ray.direction);
        glm::vec3 new_direction = hit_info->material->SampleBSDF(
            hit_info->point, view_local, beta, rng);
        glm::vec3 new_direction_world = frame.WorldFromLocal(new_direction);
        ray.origin = hit_info->point + new_direction_world * ray_origin_bias;
        ray.direction = new_direction_world;
    }

    return color;
}
