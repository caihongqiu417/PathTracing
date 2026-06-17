#include "renderer/mis_path_tracing_renderer.h"
#include "util/frame.h"
#include <cmath>


// 幂启发式 MIS 权重：w_j = p_j² / (p_j² + p_k²)
// pdf_j 越大 → 权重越接近 1；两者相等 → 权重 0.5
static float PowerHeuristic(float pdf_j, float pdf_k)
{
    return pdf_j * pdf_j / (pdf_j * pdf_j + pdf_k * pdf_k);
}


// ============================================================
// MIS 路径追踪主函数
//
// 每次弹射的直接光照采用两种策略的 MIS 组合：
//
//   策略 A（光源采样 / NEE）：
//     向光源随机采一个方向 wi_light，权重 = PowerHeuristic(p_light, p_bsdf(wi_light))
//     L += beta * f * Le * |cosθ| * w_A / (prob * p_light)
//
//   策略 B（BSDF 采样）：
//     按材质分布采下一段路径方向 wi_bsdf，若下一步命中光源
//     权重 = PowerHeuristic(p_bsdf, prob * p_light(wi_bsdf))
//     L += beta_new * Le * w_B
//     （beta_new 已将 f * |cosθ| / p_bsdf 折叠进去）
//
// delta 材质（镜面/电介质）特殊处理：
//   - 跳过 NEE（BSDF 评估为 0，策略 A 无法采到精确反射方向）
//   - 命中光源时 w_B = 1（策略 A 的 PDF 为 0，MIS 权重退化为全权重）
// ============================================================
glm::vec3 MISPathTracingRenderer::RenderPixel(const glm::ivec2& pixel_coord)
{
    auto ray = camera.GenerateRay(pixel_coord, rng.PixelOffset());

    glm::vec3 beta = {1.f, 1.f, 1.f}; // 路径累积吞吐量
    glm::vec3 L    = {0.f, 0.f, 0.f}; // 累积 radiance

    constexpr float q = 0.9f; // 俄罗斯轮盘赌继续概率

    // 上一次 BSDF 采样的 PDF，用于计算策略 B 的 MIS 权重
    // 相机射线初始化为 1（后续通过 last_is_specular 跳过权重计算）
    float bsdf_pdf = 1.f;

    // 上一步是否为 delta 材质（或相机）
    // true  → 上一步没有做 NEE，命中光源时策略 B 取全权重 w_B = 1
    // false → 上一步做了 NEE，需用 PowerHeuristic 分摊两种策略的贡献
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
        // b. 策略 B：BSDF 采样方向命中光源，计入带 MIS 权重的直接光贡献
        //
        //    贡献公式：L += beta * Le * w_B
        //    其中 beta 已折叠了 f * |cosθ| / p_bsdf，w_B 由以下两种情况决定：
        //
        //    ① last_is_specular == true（相机射线 / delta 材质弹射后）：
        //       上一步没有 NEE，策略 A 的 PDF = 0，MIS 权重退化 → w_B = 1
        //       典型路径：相机 → 漫反射 p₁ → 镜面 p₂ → 光源
        //         p₂ 是 delta，跳过 NEE；p₃ 命中光源只能由此分支处理
        //
        //    ② last_is_specular == false（漫反射弹射后）：
        //       上一步做过 NEE（策略 A），此处用 PowerHeuristic 分摊贡献
        //       w_B = PowerHeuristic(p_bsdf, prob_light * p_light)
        // ------------------------------------------------------------------
        if (hit_info->material->area_light)
        {
            glm::vec3 Le = hit_info->material->area_light->GetRadiance(
                ray.origin, hit_info->point, hit_info->normal);

            float w_bsdf;
            if (last_is_specular)
            {
                // delta 材质或相机出发：策略 A 的 PDF = 0，全权重给策略 B
                w_bsdf = 1.f;
            }
            else
            {
                // 非 delta 弹射：用 PowerHeuristic 平衡两种策略
                // ray.origin ≈ 上一个交点（偏移 1e-4 后），误差可忽略
                float prob_light = scene.GetLightSampler().GetProb(hit_info->material->area_light);
                float pdf_light  = hit_info->material->area_light->GetPDF(
                    ray.origin, hit_info->point, hit_info->normal);
                w_bsdf = PowerHeuristic(bsdf_pdf, prob_light * pdf_light);
            }

            L += beta * Le * w_bsdf;
            break; // 光源不再继续散射，终止路径
        }

        // ------------------------------------------------------------------
        // c. 俄罗斯轮盘赌：以概率 (1-q) 提前终止路径，幸存路径放大权重
        // ------------------------------------------------------------------
        if (rng.Float01() > q)
            break;
        beta /= q;

        // ------------------------------------------------------------------
        // d. 策略 A：NEE 直接光采样，加 MIS 权重（delta 材质跳过）
        //
        //    带权重的贡献公式：
        //      L += beta * f * Le * |cosθ| * w_A / (prob * p_light)
        //    其中：
        //      w_A = PowerHeuristic(prob * p_light, p_bsdf(wi_light))
        //      p_bsdf(wi_light) = material->PDF(wi_light, wo)  ← BSDF 对该方向的 PDF
        //
        //    beta 保持不变：NEE 是"额外"采样，不影响路径延伸的权重
        // ------------------------------------------------------------------
        if (!hit_info->material->IsDeltaDistribution())
        {
            // d1. 按功率比例随机选一个光源
            auto light_src = scene.GetLightSampler().Sample(rng.Float01());
            if (light_src.has_value())
            {
                // d2. 在光源上采样一个点，返回立体角 PDF
                auto light_sample = light_src->light->SampleLight(
                    hit_info->point, scene.GetRadius(), rng);

                if (light_sample.has_value())
                {
                    // d3. 阴影射线：检测 hit_point → light_point 是否被遮挡
                    float     dist          = glm::length(light_sample->light_point - hit_info->point);
                    glm::vec3 shadow_origin = hit_info->point + light_sample->light_direction * 1e-4f;
                    auto shadow_hit         = scene.Intersect(
                        Ray{shadow_origin, light_sample->light_direction}, 0.f, dist * 0.9999f);

                    if (!shadow_hit.has_value()) // 未被遮挡
                    {
                        glm::vec3 wi_local  = frame.LocalFromWorld(light_sample->light_direction);
                        float     cos_theta = wi_local.y;

                        if (cos_theta != 0.f)
                        {
                            glm::vec3 f = hit_info->material->BSDF(
                                hit_info->point, wi_local, wo_local);

                            // d4. MIS 权重：将 BSDF 对该光源方向的 PDF 与光源 PDF 对比
                            //     p_bsdf(wi_light) 由 material->PDF() 给出
                            //     组合 PDF = prob_light * p_light（选光源 × 采点）
                            float pdf_bsdf_for_nee = hit_info->material->PDF(
                                hit_info->point, wi_local, wo_local);
                            float w_nee = PowerHeuristic(
                                light_src->prob * light_sample->pdf, pdf_bsdf_for_nee);

                            L += beta * f * light_sample->Le * glm::abs(cos_theta) * w_nee
                                 / (light_src->prob * light_sample->pdf);
                        }
                    }
                }
            }
        }

        // ------------------------------------------------------------------
        // e. BSDF 采样：确定下一段路径方向，更新 beta 用于后续弹射
        //
        //    beta *= f(wo, wi) * |cosθi| / p_bsdf
        //
        //    同时保存 p_bsdf，供下一轮命中光源时计算策略 B 的 MIS 权重
        // ------------------------------------------------------------------
        auto sample_opt = hit_info->material->SampleBSDF(hit_info->point, wo_local, rng);
        if (!sample_opt.has_value())
            break;

        const auto& s = sample_opt.value();

        bsdf_pdf = s.pdf; // 保存本次采样 PDF，下一轮 b 步用于 PowerHeuristic

        float cos_theta_i = std::abs(s.light_direction.y);
        beta *= s.bsdf * cos_theta_i / s.pdf;

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
