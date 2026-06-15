#include "renderer/path_tracing_renderer.h"
#include "util/frame.h"
#include <cmath>

glm::vec3 PathTracingRenderer::RenderPixel(const glm::ivec2& pixel_coord)
{
    // -----------------------------------------------------------------------
    // 初始化
    // -----------------------------------------------------------------------
    auto ray = camera.GenerateRay(pixel_coord, rng.PixelOffset());

    glm::vec3 beta = {1, 1, 1};  // 路径累积权重（每次弹射后按 bsdf*cos/pdf 衰减）
    glm::vec3 L    = {0, 0, 0};  // 累积 radiance

    constexpr float q = 0.9f;    // 俄罗斯轮盘赌继续概率

    // -----------------------------------------------------------------------
    // 路径追踪主循环
    // -----------------------------------------------------------------------
    while (true)
    {
        // a. 求交，无交点则路径终止
        auto hit_info = scene.Intersect(ray);
        if (!hit_info.has_value())
            break;

        // b. 俄罗斯轮盘赌：以概率 (1-q) 提前终止路径
        if (rng.Float01() > q)
            break;

        // c. 累积自发光贡献
        L += beta * hit_info->material->emissive;

        // d. 轮盘赌补偿：幸存路径放大权重，保证期望无偏
        beta /= q;

        // e. 采样 BSDF：得到 f(wo,wi)、pdf(wi)、新方向 wi
        Frame     frame(hit_info->normal);
        glm::vec3 view_local = frame.LocalFromWorld(-ray.direction);

        auto sample_opt = hit_info->material->SampleBSDF(
            hit_info->point, view_local, rng);

        // 采样失败（如折射退化）则终止路径
        if (!sample_opt.has_value())
            break;

        const auto& s = sample_opt.value();

        // f. 更新路径权重：beta *= f(wo,wi) * |cos(θi)| / pdf(wi)
        //    对漫反射：(albedo/π) * cos / (cos/π) = albedo
        //    对 delta 材质（镜面/导体/电介质）：(weight/cos) * cos / 1 = weight
        float cos_theta = std::abs(s.light_direction.y);
        beta *= s.bsdf * cos_theta / s.pdf;

        // g. 更新光线，准备下一次弹射
        glm::vec3 new_dir_world = frame.WorldFromLocal(s.light_direction);
        ray.origin    = hit_info->point + new_dir_world * 1e-4f;
        ray.direction = new_dir_world;
    }

    return L;
}
