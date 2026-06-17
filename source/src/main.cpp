#include <windows.h>
#undef RGB  // windows.h 定义了 RGB(r,g,b) 宏，会和项目的 RGB 类冲突

#include "camera/camera.h"
#include "camera/film.h"
#include "renderer/debug_renderer.h"
#include "renderer/normal_renderer.h"
#include "renderer/path_tracing_renderer.h"
#include "renderer/mis_path_tracing_renderer.h"
#include "shape/model.h"
#include "shape/plane.h"
#include "shape/scene.h"
#include "shape/sphere.h"
#include "util/rgb.h"
#include "diffuse_material.h"
#include "specular_material.h"
#include "conductor_material.h"
#include "dielectric_material.h"
#include "light/area_light.h"

// ─────────────────────────────────────────────────────────────
//  场景布局（俯视）
//
//   x:  -3          0(龙侧面)       +3
//  z=-2 [镜面球]   [  🐉龙身  ]   [光滑玻璃球]
//  z= 0 [光滑金球] [← 龙全身 →]   [磨砂玻璃球]
//  z=+2 [粗糙铜球] [  🐉龙尾  ]   [漫反射白球]
//
//  龙绕 Y 轴旋转 90°，从正面能看到完整侧影
//  暖主光 (-2, 5, 2)，冷补光 (3, 4, -1)
//  相机 (0, 2, 7) 朝向 (0, 0.2, 0)
// ─────────────────────────────────────────────────────────────

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    constexpr size_t width  = 1920;
    constexpr size_t height = 1080;
    constexpr size_t spp    = 256;

    Sphere sphere{ { 0, 0, 0 }, 0.5f };
    Plane  floor_plane{ { 0, -1,  0 }, { 0,  1, 0 }, 40.f };
    Plane  back_wall  { { 0,  0, -5 }, { 0,  0, 1 }, 20.f };
    Plane  left_wall  { {-5,  0,  0 }, { 1,  0, 0 }, 20.f };
    Plane  right_wall { { 5,  0,  0 }, {-1,  0, 0 }, 20.f };

    Scene scene{};

    // ── 房间
    scene.AddShape(floor_plane, std::make_shared<DiffuseMaterial>(RGB{ 215, 210, 200 }), { 0, 0, 0 });
    scene.AddShape(back_wall,   std::make_shared<DiffuseMaterial>(RGB{ 200, 195, 188 }), { 0, 0, 0 });
    scene.AddShape(left_wall,   std::make_shared<DiffuseMaterial>(RGB{ 180,  60,  55 }), { 0, 0, 0 });
    scene.AddShape(right_wall,  std::make_shared<DiffuseMaterial>(RGB{  55,  80, 185 }), { 0, 0, 0 });

    // ── 中心主体：龙（绕 Y 轴旋转 90°，侧面完整展示）
    // 龙模型原始 Y 最小值约 -0.283，均匀缩放 3x 后底部在 -0.849
    // pos_y = -1 - (-0.849) = -0.151，使龙恰好站在地板上
    Model dragon("models/dragon_87k.obj");
    scene.AddShape(dragon,
        std::make_shared<ConductorMaterial>(
            glm::vec3{ 0.17f, 0.35f, 1.50f },   // 黄金 n
            glm::vec3{ 3.10f, 2.40f, 1.90f },   // 黄金 k
            0.08f, 0.08f),                        // 略带磨砂的抛光金
        { 0.f, -0.15f, 0.f },                    // 贴地
        { 3.f,  3.f,   3.f },                    // 均匀缩放
        { 0.f, 90.f,   0.f });                   // 旋转 90°，露出完整侧面

    // ── 左列（x = -3）：delta 镜面 / 光滑金导体 / 粗糙铜导体
    scene.AddShape(sphere,
        std::make_shared<SpecularMaterial>(glm::vec3{ 1.f, 1.f, 1.f }),
        { -3.f, -0.5f, -2.f });

    scene.AddShape(sphere,
        std::make_shared<ConductorMaterial>(
            glm::vec3{ 0.17f, 0.35f, 1.50f },
            glm::vec3{ 3.10f, 2.40f, 1.90f },
            0.05f, 0.05f),
        { -3.f, -0.5f, 0.f });

    scene.AddShape(sphere,
        std::make_shared<ConductorMaterial>(
            glm::vec3{ 0.20f, 0.90f, 1.10f },
            glm::vec3{ 3.90f, 2.60f, 2.40f },
            0.35f, 0.35f),
        { -3.f, -0.5f, 2.f });

    // ── 右列（x = +3）：光滑玻璃 / 磨砂玻璃 / 漫反射白球
    scene.AddShape(sphere,
        std::make_shared<DielectricMaterial>(1.5f, glm::vec3{ 1.f, 1.f, 1.f }, 0.03f, 0.03f),
        { 3.f, -0.5f, -2.f });

    scene.AddShape(sphere,
        std::make_shared<DielectricMaterial>(1.5f, glm::vec3{ 1.f, 1.f, 1.f }, 0.30f, 0.30f),
        { 3.f, -0.5f, 0.f });

    scene.AddShape(sphere,
        std::make_shared<DiffuseMaterial>(RGB{ 240, 238, 230 }),
        { 3.f, -0.5f, 2.f });

    // ── 光源
    // 主光：暖黄，左前上方，大面积，正面打亮金龙
    AreaLight main_light{ sphere, glm::vec3{ 8.f, 7.f, 4.f }, true };
    auto main_light_mat = std::make_shared<DiffuseMaterial>(glm::vec3{ 1, 1, 1 });
    scene.AddAreaLight(&main_light, main_light_mat, { -2.f, 5.f, 2.f }, { 2.f, 2.f, 2.f });

    // 补光：冷蓝，右上，小面积（体现 MIS 对小光源的优势），勾勒龙的轮廓
    AreaLight fill_light{ sphere, glm::vec3{ 2.f, 4.f, 12.f }, true };
    auto fill_light_mat = std::make_shared<DiffuseMaterial>(glm::vec3{ 1, 1, 1 });
    scene.AddAreaLight(&fill_light, fill_light_mat, { 3.f, 4.f, -1.f }, { 0.5f, 0.5f, 0.5f });

    scene.Build();

    const glm::vec3 cam_pos    = {  0.f,  2.f,  7.f };
    const glm::vec3 cam_target = {  0.f,  0.2f,  0.f };
    constexpr float fovy       = 40.f;

    // 路径追踪主图
    {
        Film film{ width, height };
        Camera camera{ film, cam_pos, cam_target, fovy };
        MISPathTracingRenderer renderer{ camera, scene };
        renderer.Render(spp, "Output.ppm");
    }

    // 法线图
    {
        Film film{ width, height };
        Camera camera{ film, cam_pos, cam_target, fovy };
        NormalRenderer renderer{ camera, scene };
        renderer.Render(1, "Normal.ppm");
    }

    // BVH 包围盒测试次数热力图
    {
        Film film{ width, height };
        Camera camera{ film, cam_pos, cam_target, fovy };
        BoundsTestRenderer renderer{ camera, scene };
        renderer.Render(4, "BoundsTest.ppm");
    }

    // 三角形测试次数热力图
    {
        Film film{ width, height };
        Camera camera{ film, cam_pos, cam_target, fovy };
        TriangleTestRenderer renderer{ camera, scene };
        renderer.Render(4, "TriangleTest.ppm");
    }

    // BVH 遍历深度热力图
    {
        Film film{ width, height };
        Camera camera{ film, cam_pos, cam_target, fovy };
        BoundsDepthRenderer renderer{ camera, scene };
        renderer.Render(4, "BoundsDepth.ppm");
    }

    return 0;
}
