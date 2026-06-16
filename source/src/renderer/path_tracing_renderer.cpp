#include "renderer/path_tracing_renderer.h"
#include "util/frame.h"
#include <cmath>

// ============================================================
// 路径追踪主函数
//
// 每次弹射做两件事：
//   1. NEE（直接光采样）：朝光源随机采一个方向，算直接贡献
//   2. BSDF 采样：按材质分布采下一段路径方向
//
// 发光贡献只在 last_is_specular == true 时计入，避免与 NEE 重复计算：
//   - 相机射线或上一次弹射是 delta 材质时，直接命中光源才算
//   - 漫反射弹射后命中光源的贡献已由上一轮 NEE 算过，此处跳过
// ============================================================
glm::vec3 PathTracingRenderer::RenderPixel(const glm::ivec2& pixel_coord)
{
    auto ray = camera.GenerateRay(pixel_coord, rng.PixelOffset());

    glm::vec3 beta = {1.f, 1.f, 1.f}; // 路径累积权重
    glm::vec3 L    = {0.f, 0.f, 0.f}; // 累积 radiance

    constexpr float q = 0.9f; // 俄罗斯轮盘赌继续概率

    // 相机射线视为从 delta 点出发，首次命中光源需计入发光
    bool last_is_specular = true;

    while (true)
    {
        // ------------------------------------------------------------------
        // a. 求交
        // ------------------------------------------------------------------
        auto hit_info = scene.Intersect(ray);
        if (!hit_info.has_value())
            break;

        Frame     frame(hit_info->normal);
        glm::vec3 wo_local = frame.LocalFromWorld(-ray.direction); // 出射方向（局部坐标）

        // ------------------------------------------------------------------
        // b. BSDF 采样方向命中光源时，计入发光贡献
        //
        //    典型场景：相机 → 漫反射 p₁ → 镜面 p₂ → 光源 p₃
        //      - p₁ 的 NEE 被镜面遮挡，采不到光源
        //      - p₂ 是 delta 材质，跳过 NEE
        //      - p₃ 命中光源：这是整条路径唯一能计入 Le 的机会
        //
        //    条件：last_is_specular && hit_info->material->area_light（上一步没有做 NEE且这次打到光源）
        //    反之漫反射后不计入，因为上一步 NEE 已经算过这段贡献，避免重复
        // ------------------------------------------------------------------
        if (last_is_specular && hit_info->material->area_light)
        {
            glm::vec3 Le = hit_info->material->area_light->GetRadiance(
                ray.origin, hit_info->point, hit_info->normal);
            L += beta * Le;
            break; // 光源不继续散射，终止路径
        }

        // ------------------------------------------------------------------
        // c. 俄罗斯轮盘赌：以概率 (1-q) 提前终止路径，幸存路径放大权重
        // ------------------------------------------------------------------
        if (rng.Float01() > q)
            break;
        beta /= q;

        // ------------------------------------------------------------------
        // d. NEE 直接光采样（delta 材质跳过）
        //
        //    直接把贡献加进 L，不更新 beta：
        //      L += beta * f(wi,wo) * Le * |cosθ| / (prob * pdf_light)
        //
        //    beta 保持不变，NEE 和 BSDF 采样完全独立，互不影响
        // ------------------------------------------------------------------
        if (!hit_info->material->IsDeltaDistribution())
        {
            // d1. 按功率比例选一个光源
            auto light_src = scene.GetLightSampler().Sample(rng.Float01());
            if (light_src.has_value())
            {
                // d2. 在光源上采样一个点
                auto light_sample = light_src->light->SampleLight(
                    hit_info->point, scene.GetRadius(), rng);

                if (light_sample.has_value())
                {
                    // d3. 阴影射线：检测 hit_point → light_point 是否被遮挡
                    // origin 偏移 1e-4 避免自交，tmax 用 dist*0.9999 避免命中光源自身
                    float     dist          = glm::length(light_sample->light_point - hit_info->point);
                    glm::vec3 shadow_origin = hit_info->point + light_sample->light_direction * 1e-4f;
                    auto shadow_hit = scene.Intersect(
                        Ray{shadow_origin, light_sample->light_direction}, 0.f, dist * 0.9999f);

                    if (!shadow_hit.has_value()) // 未被遮挡
                    {
                        glm::vec3 wi_local  = frame.LocalFromWorld(light_sample->light_direction);
                        float     cos_theta = wi_local.y;

                        // cos_theta < 0 表示光从背面来（透射），也允许通过，BSDF 自行处理
                        if (cos_theta != 0.f)
                        {
                            glm::vec3 f = hit_info->material->BSDF(
                                hit_info->point, wi_local, wo_local);

                            // 直接加入 L，beta 不变
                            L += beta * f * light_sample->Le * glm::abs(cos_theta)
                                 / (light_src->prob * light_sample->pdf);
                        }
                    }
                }
            }
        }

        // ------------------------------------------------------------------
        // e. BSDF 采样：确定下一段路径方向，更新 beta 用于后续弹射
        //
        //    不直接加 L，只更新 beta：
        //      beta *= f(wo,wi) * |cosθi| / pdf_bsdf
        //
        //    beta 累积了从相机到当前点的所有 BSDF * cos / pdf 的乘积，
        //    下一次弹射命中光源时，L += beta * Le 才能得到完整贡献
        // ------------------------------------------------------------------
        auto sample_opt = hit_info->material->SampleBSDF(hit_info->point, wo_local, rng);
        if (!sample_opt.has_value())
            break;

        const auto& s = sample_opt.value();

        float cos_theta_i = std::abs(s.light_direction.y);
        beta *= s.bsdf * cos_theta_i / s.pdf; // 更新路径权重，不加 L

        // ------------------------------------------------------------------
        // f. 更新状态，准备下一次弹射
        // ------------------------------------------------------------------
        last_is_specular = hit_info->material->IsDeltaDistribution();

        glm::vec3 new_dir_world = frame.WorldFromLocal(s.light_direction);
        ray.origin    = hit_info->point + new_dir_world * 1e-4f;
        ray.direction = new_dir_world;
    }

    return L;
}
