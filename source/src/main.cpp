#include <windows.h>
#undef RGB  // windows.h 定义了 RGB(r,g,b) 宏，会和项目的 RGB 类冲突
#include "camera/camera.h"
#include "camera/film.h"
#include "renderer/debug_renderer.h"
#include "renderer/normal_renderer.h"
#include "renderer/rt_renderer.h"
#include "renderer/path_tracing_renderer.h"
#include "shape/model.h"
#include "shape/plane.h"
#include "shape/scene.h"
#include "shape/sphere.h"
#include "util/rgb.h"
#include "diffuse_material.h"
#include "specular_material.h"
#include "dielectric_material.h"
#include "conductor_material.h"

// =========================
// 程序入口
// =========================

int main()
{
	SetConsoleOutputCP(CP_UTF8);
	constexpr size_t width = 1920;
	constexpr size_t height = 1080;
	constexpr size_t spp = 64;

	Sphere sphere{ { 0, 0, 0 }, .5f };
	Plane ground{ { 0, -1, 0 }, { 0, 1, 0 } };
	Plane back_wall{ { 0, 0, -3 }, { 0, 0, 1 } };
	Plane glass_wall{ { 0, 0, 0 }, { -1, 0, 0 } };

	Scene scene{};

	// -----------------------------------------------------------------------
	// 电介质测试场景
	// -----------------------------------------------------------------------

	// 地面：漫反射，透过玻璃能看到底色
	scene.AddShape(ground,
		std::make_shared<DiffuseMaterial>(RGB{ 80, 120, 80 }),
		{ 0, 0, 0 });

	// 背景墙：漫反射，方便观察折射变形
	scene.AddShape(back_wall,
		std::make_shared<DiffuseMaterial>(RGB{ 180, 60, 60 }),
		{ 0, 0, 0 });

	// 中央：抛光玻璃 (ior=1.5, roughness=0.05) — 近似光滑，背景折射清晰
	scene.AddShape(sphere,
		std::make_shared<DielectricMaterial>(1.5f, glm::vec3{ 1, 1, 1 }, 0.05f, 0.05f),
		{ 0, 0, 0 });

	// 左侧：磨砂水 (ior=1.33, roughness=0.25) — 轻微模糊，折射像涟漪水面
	scene.AddShape(sphere,
		std::make_shared<DielectricMaterial>(1.33f, glm::vec3{ 1, 1, 1 }, 0.25f, 0.25f),
		{ -0.8f, -0.35f, 0.8f }, { 0.6f, 0.6f, 0.6f });

	// 右侧：磨砂玻璃 (ior=2.0, roughness=0.5) — 明显磨砂，背景几乎全部散开
	scene.AddShape(sphere,
		std::make_shared<DielectricMaterial>(2.0f, glm::vec3{ 1, 1, 1 }, 0.5f, 0.5f),
		{ 1.6f, -0.35f, -0.8f }, { 0.6f, 0.6f, 0.6f });

	// 后方：磨砂蓝色玻璃 (ior=1.5, roughness=0.35)
	scene.AddShape(sphere,
		std::make_shared<DielectricMaterial>(
			1.5f,
			glm::vec3{ 1, 1, 1 },
			glm::vec3{ 0.7f, 0.85f, 1.0f },
			0.35f, 0.35f),
		{ 0, 0.5f, -1.2f }, { 0.5f, 0.5f, 0.5f });

	// 竖直玻璃平面（暂时注释掉，先测试球体效果）
	// scene.AddShape(glass_wall,
	// 	std::make_shared<DielectricMaterial>(1.5f, glm::vec3{ 1, 1, 1 }),
	// 	{ -1.5f, 0, 0 });

	// -----------------------------------------------------------------------
	// 导体测试场景（紧靠电介质球右侧，便于对比）
	// -----------------------------------------------------------------------

	// 导体球：沿 x 轴展开（-2 / 0 / +2），抬高到 y=1 浮于玻璃球上方
	// z=0 与玻璃球同深度，避免和相机 z=2 挤在一起

	// 红色金属，polished (roughness=0.05) — 近镜面，可见清晰高光点
	scene.AddShape(sphere,
		std::make_shared<ConductorMaterial>(
			glm::vec3{ 0.2f, 1.0f, 1.0f },
			glm::vec3{ 3.5f, 0.1f, 0.1f },
			0.05f, 0.05f),
		{ -2.f, 1.f, 0.f });

	// 绿色金属，satin (roughness=0.3) — 拉长的模糊高光，磨砂金属感
	scene.AddShape(sphere,
		std::make_shared<ConductorMaterial>(
			glm::vec3{ 1.0f, 0.2f, 1.0f },
			glm::vec3{ 0.1f, 3.5f, 0.1f },
			0.3f, 0.3f),
		{ 0.f, 1.f, 0.f });

	// 蓝色金属，rough (roughness=0.6) — 高光极宽，接近漫反射金属
	scene.AddShape(sphere,
		std::make_shared<ConductorMaterial>(
			glm::vec3{ 1.0f, 1.0f, 0.2f },
			glm::vec3{ 0.1f, 0.1f, 3.5f },
			0.6f, 0.6f),
		{ 2.f, 1.f, 0.f });

	// 光源：放在场景上方，对所有物体都有好的可见度
	auto light = std::make_shared<DiffuseMaterial>(glm::vec3{ 1, 1, 1 });
	light->SetEmissive({ 10.f, 8.f, 4.f });
	scene.AddShape(sphere, light, { 0.f, 3.f, 1.f }, { 1.5f, 1.5f, 1.5f });

	// -----------------------------------------------------------------------
	// 原中世纪场景（暂时注释）
	// -----------------------------------------------------------------------
	// Model model("models/dragon_87k.obj");
	// scene.AddShape(model,
	// 	std::make_shared<DiffuseMaterial>(RGB{190, 45, 25}),
	// 	{ 0, 0, 0 }, { 1, 3, 2 });
	// scene.AddShape(sphere,
	// 	std::make_shared<DiffuseMaterial>(RGB{230, 185, 40}),
	// 	{ 0, 0, 2.5 });
	// scene.AddShape(sphere,
	// 	std::make_shared<DiffuseMaterial>(RGB{230, 185, 40}),
	// 	{ 0, 0, -2.5 });
	// scene.AddShape(sphere,
	// 	std::make_shared<SpecularMaterial>(glm::vec3{1, 1, 1}),
	// 	{ 3, 0.5, -2 });
	// scene.AddShape(sphere,
	// 	std::make_shared<SpecularMaterial>(glm::vec3{1, 1, 1}),
	// 	{ 3, 0.5,  2 });
	// auto warm_light = std::make_shared<DiffuseMaterial>(glm::vec3{1, 1, 1});
	// warm_light->SetEmissive({ 7.f, 4.f, 0.8f });
	// scene.AddShape(sphere, warm_light, { -2, 3, 0 }, { 3, 3, 3 });
	// auto cool_light = std::make_shared<DiffuseMaterial>(glm::vec3{1, 1, 1});
	// cool_light->SetEmissive({ 0.6f, 1.0f, 3.5f });
	// scene.AddShape(sphere, cool_light, { 5, 3, -3 }, { 2, 2, 2 });
	// scene.AddShape(plane,
	// 	std::make_shared<DiffuseMaterial>(RGB{140, 130, 120}),
	// 	{ 0, -0.5, 0 });

	scene.Build();

	// 路径追踪图
	Film pt_film{ width, height };
	Camera pt_camera{ pt_film, { -4.f, 0.5f, 2.f }, { 0, 0, 0 }, 45.f };
	PathTracingRenderer pt_renderer{ pt_camera, scene };
	pt_renderer.Render(spp, "Output.ppm");

	// 简单光追图
	//Film rt_film{ width, height };
	//Camera rt_camera{ rt_film, { -3.6, 0, 0 }, { 0, 0, 0 }, 45.f };
	//RTRenderer rt_renderer{ rt_camera, scene };
	//rt_renderer.Render(spp, "Output_rt.ppm");

	// Debug 热力图
	//Film bounds_test_film{ width, height };
	//Camera bounds_test_camera{ bounds_test_film, { -3.6, 0, 0 }, { 0, 0, 0 }, 45.f };
	//BoundsTestRenderer bounds_test_renderer{ bounds_test_camera, scene };
	//bounds_test_renderer.Render(4, "BoundsTest.ppm");

	//Film triangle_test_film{ width, height };
	//Camera triangle_test_camera{ triangle_test_film, { -3.6, 0, 0 }, { 0, 0, 0 }, 45.f };
	//TriangleTestRenderer triangle_test_renderer{ triangle_test_camera, scene };
	//triangle_test_renderer.Render(4, "TriangleTest.ppm");

	//Film bounds_depth_film{ width, height };
	//Camera bounds_depth_camera{ bounds_depth_film, { -3.6, 0, 0 }, { 0, 0, 0 }, 45.f };
	//BoundsDepthRenderer bounds_depth_renderer{ bounds_depth_camera, scene };
	//bounds_depth_renderer.Render(4, "BoundsDepth.ppm");

	// 法线图
	//Film normal_film{ width, height };
	//Camera normal_camera{ normal_film, { -3.6, 0, 0 }, { 0, 0, 0 }, 45.f };
	//NormalRenderer normal_renderer{ normal_camera, scene };
	//normal_renderer.Render(1, "Normal.ppm");

	return 0;
}
