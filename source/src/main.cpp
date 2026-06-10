#include <atomic>
#include <chrono>
#include <iostream>
#include <random>
#include <thread>

#include <glm/glm.hpp>

#include "camera.h"
#include "film.h"
#include "hit_info.h"
#include "shape.h"
#include "sphere.h"
#include "model.h"
#include"plane.h"
#include "scene.h"
#include"frame.h"
#include"rgb.h"
#include "thread_pool.h"

// =============================================================================
// 终端进度条 [=========>          ] 42%
// 仅由【主线程】调用
// =============================================================================

static void PrintRenderProgress(size_t done, size_t total)
{
	constexpr int bar_width = 40;

	const float progress = static_cast<float>(done) / static_cast<float>(total);
	const int filled = static_cast<int>(progress * bar_width);

	std::cout << '\r' << "[";
	for (int i = 0; i < bar_width; ++i)
	{
		if (i < filled)
			std::cout << '=';
		else if (i == filled)
			std::cout << '>';
		else
			std::cout << ' ';
	}
	std::cout << "] " << static_cast<int>(progress * 100.f) << "%" << std::flush;
}

// =============================================================================
// 程序入口
// 【主线程】   刷进度 + Wait + Save
// 【入队线程】 ParallerlFor
// 【Worker】   算像素
// =============================================================================

int main()
{
	constexpr int max_bounces = 8;
	constexpr float ray_origin_bias = 1e-4f;

	// --- 【主线程】场景初始化 ---

	ThreadPool thread_pool{};

	Film film{ 1920, 1080 };
	Camera camera{ film, { -3.6, 0, 0}, { 0, 0, 0 }, 45.f };

	Model model("models/simple_dragon.obj");
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

	scene.AddShape(plane,{{1,1,1},false,RGB{128,128,255}},{0,-0.5,0});
	glm::vec3 light_pos{ -1, 2, 1 };

	// --- 【跨线程共享】进度计数 ---

	const size_t total_pixels = film.GetWidth() * film.GetHeight();
	std::atomic<size_t> pixels_done{ 0 };
	std::atomic<bool> enqueue_done{ false };

	// --- 【入队线程】提交任务 ---

	std::thread enqueue_thread([&]()
		{
			thread_pool.ParallerlFor(film.GetWidth(), film.GetHeight(), [&](size_t x, size_t y)
				{
					// ↓↓↓ 【Worker 线程】执行 ↓↓↓

					// 每个像素使用独立随机数引擎，避免多个线程共享同一个 mt19937。
					std::mt19937 gen(static_cast<std::mt19937::result_type>(23451334u + y * film.GetWidth() + x));
					std::uniform_real_distribution<float> uniform(-1.f, 1.f);

					// 最终反照率
					glm::vec3 beta = { 1,1,1 };
					glm::vec3 color = { 0,0,0 };

					// 若 shape 是 Scene：内部会光线从世界空间转换到模型空间，然后与模型空间内的场景求交，再将交点转回世界空间
					Ray ray = camera.GenerateRay({ x, y });

					// 限制最大反弹次数，避免少数光线在场景里长时间循环导致进度卡住。
					for (int bounce = 0; bounce < max_bounces; ++bounce)
					{
						auto hit_info = scene.Intersect(ray);
						if (hit_info.has_value())
						{
							//--- 计算颜色 ---
							
							color += beta * hit_info->material->emissive;//计算反射后的颜色
							
							beta *= hit_info->material->albedo;//更新衰减率

							//--- 计算反射后的方向 ---
							// 沿法线稍微抬起一点，避免新射线立刻再次命中当前表面。
							ray.origin = hit_info->point + hit_info->normal * ray_origin_bias;
							
							Frame frame(hit_info->normal);//构建TBN矩阵

							glm::vec3 light_direction;
							if (hit_info->material->is_specular/*1.镜面反射*/)
							{
								//将射线取反，然后绕y轴转180°=>得到反射光方向
								glm::vec3 view_direction = frame.LocalFromWorld(-ray.direction);
								light_direction = { -view_direction.x,view_direction.y,-view_direction.z };
							}
							else {/*2.漫反射*/
								do {
									//拒绝采样:生成半球上的反射光方向
									light_direction = glm::vec3(uniform(gen), uniform(gen), uniform(gen));
								} while (glm::length(light_direction) > 1);
								if (light_direction.y < 0) {
									light_direction.y = -light_direction.y;
								}
							}
							//将局部空间内的反射光方向转换到世界坐标系
							ray.direction = frame.WorldFromLocal(light_direction);
						}
						else
						{
							break;
						}
					}
					film.SetPixel(x, y, color);

					pixels_done.fetch_add(1, std::memory_order_relaxed);
				});

			enqueue_done.store(true, std::memory_order_release);
		});

	// --- 【主线程】刷新进度条 ---

	while (!enqueue_done.load(std::memory_order_acquire)
		|| pixels_done.load(std::memory_order_relaxed) < total_pixels)
	{
		PrintRenderProgress(pixels_done.load(std::memory_order_relaxed), total_pixels);
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	enqueue_thread.join();
	PrintRenderProgress(total_pixels, total_pixels);
	std::cout << std::endl;

	thread_pool.Wait();

	film.Save("Output.ppm");

	return 0;
}
