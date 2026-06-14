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

// =========================
// 程序入口
// =========================

int main()
{
	SetConsoleOutputCP(CP_UTF8);
	constexpr size_t width = 1920;
	constexpr size_t height = 1080;
	constexpr size_t spp = 64;

	// 场景：中世纪暗夜风格
	// 配色思路：暖金烛光 vs 冷蓝月光，形成冷暖对比；龙用深朱红，地面用深灰石板
	Model model("models/dragon_87k.obj");
	Sphere sphere{ { 0, 0, 0 }, .5f };
	Plane plane{ {0,0,0},{0,1,0} };

	Scene scene{};

	// 主体：朱红色龙
	scene.AddShape(model,
		{ RGB{190, 45, 25} },
		{ 0, 0, 0 }, { 1, 3, 2 });

	// 两侧：明亮金色装饰球
	scene.AddShape(sphere,
		{ RGB{230, 185, 40} },
		{ 0, 0, 2.5 });
	scene.AddShape(sphere,
		{ RGB{230, 185, 40} },
		{ 0, 0, -2.5 });

	// 两侧：镜面球
	scene.AddShape(sphere,
		{ {1,1,1}, true },
		{ 3, 0.5, -2 });
	scene.AddShape(sphere,
		{ {1,1,1}, true },
		{ 3, 0.5,  2 });

	// 主光：暖金烛光，y=3 更接近场景，地面和球都能被照亮
	scene.AddShape(sphere,
		{ {1,1,1}, false, {7.f, 4.f, 0.8f} },
		{ -2, 3, 0 }, { 3, 3, 3 });

	// 补光：冷蓝月光，从右上方打来，和烛光形成冷暖对比
	scene.AddShape(sphere,
		{ {1,1,1}, false, {0.6f, 1.0f, 3.5f} },
		{ 5, 3, -3 }, { 2, 2, 2 });

	// 地面：中灰石板，线性值约 0.25，能明显接收光照
	scene.AddShape(plane,
		{ RGB{140, 130, 120} },
		{ 0, -0.5, 0 });

	scene.Build();

	// 路径追踪图
	Film pt_film{ width, height };
	Camera pt_camera{ pt_film, { -3.6, 0, 0 }, { 0, 0, 0 }, 45.f };
	PathTracingRenderer pt_renderer{ pt_camera, scene };
	pt_renderer.Render(spp, "Output.ppm");

	// 简单光追图
	//Film rt_film{ width, height };
	//Camera rt_camera{ rt_film, { -3.6, 0, 0 }, { 0, 0, 0 }, 45.f };
	//RTRenderer rt_renderer{ rt_camera, scene };
	//rt_renderer.Render(spp, "Output_rt.ppm");

	// Debug 热力图
	Film bounds_test_film{ width, height };
	Camera bounds_test_camera{ bounds_test_film, { -3.6, 0, 0 }, { 0, 0, 0 }, 45.f };
	BoundsTestRenderer bounds_test_renderer{ bounds_test_camera, scene };
	bounds_test_renderer.Render(4, "BoundsTest.ppm");

	Film triangle_test_film{ width, height };
	Camera triangle_test_camera{ triangle_test_film, { -3.6, 0, 0 }, { 0, 0, 0 }, 45.f };
	TriangleTestRenderer triangle_test_renderer{ triangle_test_camera, scene };
	triangle_test_renderer.Render(4, "TriangleTest.ppm");

	Film bounds_depth_film{ width, height };
	Camera bounds_depth_camera{ bounds_depth_film, { -3.6, 0, 0 }, { 0, 0, 0 }, 45.f };
	BoundsDepthRenderer bounds_depth_renderer{ bounds_depth_camera, scene };
	bounds_depth_renderer.Render(4, "BoundsDepth.ppm");

	// 法线图
	Film normal_film{ width, height };
	Camera normal_camera{ normal_film, { -3.6, 0, 0 }, { 0, 0, 0 }, 45.f };
	NormalRenderer normal_renderer{ normal_camera, scene };
	normal_renderer.Render(1, "Normal.ppm");
	



	return 0;
}
