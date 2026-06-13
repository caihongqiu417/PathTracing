#include <windows.h>
#include "camera/camera.h"
#include "camera/film.h"
#include "renderer/debug_renderer.h"
#include "renderer/normal_renderer.h"
#include "renderer/rt_renderer.h"
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
	constexpr size_t spp = 16;

	// 场景
	Model model("models/dragon_87k.obj");
	Sphere sphere{ { 0, 0, 0 }, .5f };
	Plane plane{ {0,0,0},{0,1,0} };

	Scene scene{};
	scene.AddShape(model, {}, { 0, 0, 0 }, { 1,3,2 });
	scene.AddShape(
		sphere,
		{ { 1, 1, 1 }, false, RGB(255, 128, 128) },
		{ 0, 0, 2.5 }
	);

	scene.AddShape(
	    sphere,
	    { { 1, 1, 1 }, false, RGB(128, 128, 255) },
	    { 0, 0, -2.5 }
	);

	scene.AddShape(
		sphere,
		{ { 1, 1, 1 }, true },
		{ 3, 0.5, -2}
	);

	scene.AddShape(plane, { {1,1,1},false,RGB{128,128,255} }, { 0,-0.5,0 });
	scene.Build();

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
	
	// 路径追踪图
	Film rt_film{ width, height };
	Camera rt_camera{ rt_film, { -3.6, 0, 0 }, { 0, 0, 0 }, 45.f };
	RTRenderer rt_renderer{ rt_camera, scene };
	rt_renderer.Render(spp, "Output.ppm");



	return 0;
}
