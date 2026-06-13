#include "renderer/rt_renderer.h"

#include "util/frame.h"
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
        beta *= hit_info->material->albedo;             // 乘反射率

        // 3. 生成新光线
        ray.origin = hit_info->point + hit_info->normal * ray_origin_bias;  // 新起点

        Frame frame(hit_info->normal);                  // 局部坐标系
        glm::vec3 light_direction;

        if (hit_info->material->is_specular)
        {
            // 镜面反射
            glm::vec3 view_direction = frame.LocalFromWorld(-ray.direction);
            light_direction = { -view_direction.x, view_direction.y, -view_direction.z };
        }
        else
        {
            // 漫反射（半球采样）
            light_direction = rng.SampleHemisphere();
        }

        ray.direction = frame.WorldFromLocal(light_direction);  // 新方向
    }

    return color;
}
