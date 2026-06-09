#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include <glm/glm.hpp>

#include "camera.h"
#include "film.h"
#include "hit_info.h"
#include "shape.h"
#include "sphere.h"
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
	// --- 【主线程】场景初始化 ---

	ThreadPool thread_pool{};
	Film film{ 1920, 1080 };
	Camera camera{ film, { 0, 0, 1 }, { 0, 0, 0 }, 90.f };
	Sphere sphere{ { 0, 0, 0 }, .5f };
	Shape& shape = sphere;  // 多态：后续可换成其它 Shape 子类
	glm::vec3 light_pos{ 1, 1, 1 };

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

					Ray ray = camera.GenerateRay({ x, y });
					std::optional<HitInfo> hit = shape.Intersect(ray);

					if (hit.has_value())
					{
						const HitInfo& hit_info = hit.value();
						glm::vec3 L = glm::normalize(light_pos - hit_info.point);
						float cosine = glm::max(0.f, glm::dot(hit_info.normal, L));
						film.SetPixel(x, y, { cosine, cosine, cosine });
					}

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
