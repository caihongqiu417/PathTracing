#include <windows.h>
#undef RGB

#include "camera/camera.h"
#include "camera/film.h"
#include "renderer/path_tracing_renderer.h"
#include "renderer/mis_path_tracing_renderer.h"
#include "shape/plane.h"
#include "shape/scene.h"
#include "shape/sphere.h"
#include "util/rgb.h"
#include "diffuse_material.h"
#include "conductor_material.h"
#include "light/area_light.h"

int main()
{
    SetConsoleOutputCP(CP_UTF8);

    constexpr size_t width  = 1280;
    constexpr size_t height = 720;
    constexpr size_t spp    = 64;

    Sphere unit_sphere{ {0, 0, 0}, 0.5f };
    Plane  floor_plane{ {0, -1,  0}, {0,  1, 0}, 30.f };
    Plane  back_plane { {0,  0, -10}, {0,  0, 1}, 30.f };
    Plane  left_plane { {-5, 0,  0}, { 1, 0, 0}, 30.f };
    Plane  right_plane{ { 5, 0,  0}, {-1, 0, 0}, 30.f };

    Scene scene{};

    // ── 地面：光滑导体，roughness=0.08，BSDF 峰值窄（约 ±8°）
    scene.AddShape(floor_plane,
        std::make_shared<ConductorMaterial>(
            glm::vec3{ 2.4f, 2.4f, 2.4f },
            glm::vec3{ 3.4f, 3.4f, 3.4f },
            0.08f, 0.08f),
        { 0, 0, 0 });

    // ── 后墙、左右墙
    scene.AddShape(back_plane,
        std::make_shared<DiffuseMaterial>(RGB{ 180, 180, 175 }),
        { 0, 0, 0 });
    scene.AddShape(left_plane,
        std::make_shared<DiffuseMaterial>(RGB{ 185, 55, 50 }),
        { 0, 0, 0 });
    scene.AddShape(right_plane,
        std::make_shared<DiffuseMaterial>(RGB{ 50, 75, 190 }),
        { 0, 0, 0 });

    // ── 漫反射球（左侧，参照）：两种渲染器结果相近
    scene.AddShape(unit_sphere,
        std::make_shared<DiffuseMaterial>(RGB{ 240, 200, 120 }),
        { -2.5f, -0.5f, -1.f });

    // ── 金色导体球（右侧）：roughness=0.08，同样受益于 MIS
    scene.AddShape(unit_sphere,
        std::make_shared<ConductorMaterial>(
            glm::vec3{ 0.17f, 0.35f, 1.5f },
            glm::vec3{ 3.1f,  2.4f, 1.9f },
            0.08f, 0.08f),
        { 2.5f, -0.5f, -1.f });

    // ── 大背景光：(0, 0, -7)，scale=2 → 球半径 1
    //    经过精确计算，此光源位于地面镜面反射方向上（偏差 <3°）
    //    相机 → 地面的反射方向 ≈ (0, 0.275, -0.961)
    //    地面中心 → 光源方向  ≈ (0, 0.293, -0.956)
    AreaLight main_light{ unit_sphere, glm::vec3{ 0.8f, 0.8f, 0.65f }, false };
    auto main_light_mat = std::make_shared<DiffuseMaterial>(glm::vec3{ 1, 1, 1 });
    scene.AddAreaLight(&main_light, main_light_mat, { 0.f, 0.f, -7.f }, { 2.f, 2.f, 2.f });

    // ── 小补光：右上方，高亮度、极小立体角
    //    NEE 容易定位，BSDF 几乎采不到，两种渲染器对此光源表现一致
    AreaLight fill_light{ unit_sphere, glm::vec3{ 18.f, 14.f, 8.f }, false };
    auto fill_light_mat = std::make_shared<DiffuseMaterial>(glm::vec3{ 1, 1, 1 });
    scene.AddAreaLight(&fill_light, fill_light_mat, { 3.f, 2.5f, 2.f }, { 0.1f, 0.1f, 0.1f });

    scene.Build();

    const glm::vec3 cam_pos    = { 0.f, 1.5f, 5.f };
    const glm::vec3 cam_target = { 0.f, -0.5f, -2.f };
    constexpr float fovy       = 55.f;

    {
        Film film{ width, height };
        Camera camera{ film, cam_pos, cam_target, fovy };
        PathTracingRenderer renderer{ camera, scene };
        renderer.Render(spp, "Compare_NEE.ppm");
    }
    {
        Film film{ width, height };
        Camera camera{ film, cam_pos, cam_target, fovy };
        MISPathTracingRenderer renderer{ camera, scene };
        renderer.Render(spp, "Compare_MIS.ppm");
    }

    return 0;
}
