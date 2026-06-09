#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include <glm/glm.hpp>

#include "camera.h"
#include "film.h"
#include "sphere.h"
#include "thread_pool.h"

// =============================================================================
// 终端进度条 [=========>          ] 42%
// =============================================================================

/// 在同一行刷新 ASCII 进度条；仅由【主线程】调用
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
// =============================================================================
//
// 渲染阶段共有三类线程（时间线）：
//
//   【主线程】     初始化 → 启动入队线程 → 刷进度条 → join → Wait → Save
//   【入队线程】   ParallelFor 循环，把每个像素包装成 Task 推入队列
//   【Worker×N】  从队列取 Task，执行 lambda（射线/求交/shading），pixels_done++
//
//        主线程              入队线程                 Worker 0..N-1
//          │                    │                         │
//   创建 ThreadPool ────────────┼────────────────────────→ 启动，空转等任务
//          │                    │                         │
//          ├─ spawn ──────────→ ParallerlFor              │
//          │                    ├─ AddTask ─────────────→ GetTask → run()
//          ├─ 刷进度条 ←──────────┼──────────────────────── pixels_done++
//          │   (读 pixels_done)   │                         │
//          │                    └─ enqueue_done = true     │
//          ├─ join 入队线程        │                         │
//          ├─ Wait() ←─────────────┼──────────────────────── pending_task_count--
//          └─ Save()              │                         │
//
// =============================================================================

int main()
{
	// --- 【主线程】场景初始化 ---

	ThreadPool thread_pool{};  // 构造时启动 Worker×N，见 thread_pool.cpp
	Film film{ 1920, 1080 };
	Camera camera{ film, { 0, 0, 1 }, { 0, 0, 0 }, 90.f };
	Sphere sphere{ { 0, 0, 0 }, .5f };
	glm::vec3 light_pos{ 1, 1, 1 };

	// --- 【跨线程共享】进度计数 ---

	const size_t total_pixels = film.GetWidth() * film.GetHeight();
	std::atomic<size_t> pixels_done{ 0 };     // Worker 写，主线程读
	std::atomic<bool> enqueue_done{ false };  // 入队线程写，主线程读

	// --- 【入队线程】提交任务 ---
	// 只做：双重 for 循环 + AddTask；不执行 lambda 里的渲染逻辑

	std::thread enqueue_thread([&]()
		{
			thread_pool.ParallerlFor(film.GetWidth(), film.GetHeight(), [&](size_t x, size_t y)
				{
					// ↓↓↓ 以下在【Worker 线程】执行，不在入队线程 ↓↓↓

					Ray ray = camera.GenerateRay({ x, y });
					auto result = sphere.Intersect(ray);

					if (result.has_value())
					{
						glm::vec3 hit_point = ray.Hit(result.value());
						glm::vec3 normal = glm::normalize(hit_point - sphere.center);
						glm::vec3 L = glm::normalize(light_pos - hit_point);
						float cosine = glm::max(0.f, glm::dot(normal, L));
						film.SetPixel(x, y, { cosine, cosine, cosine });
					}

					pixels_done.fetch_add(1, std::memory_order_relaxed);
				});

			enqueue_done.store(true, std::memory_order_release);
		});

	// --- 【主线程】刷新进度条 ---
	// 与入队、渲染并行；读 pixels_done，不负责算像素

	while (!enqueue_done.load(std::memory_order_acquire)
		|| pixels_done.load(std::memory_order_relaxed) < total_pixels)
	{
		PrintRenderProgress(pixels_done.load(std::memory_order_relaxed), total_pixels);
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	enqueue_thread.join();  // 【主线程】等待入队线程结束
	PrintRenderProgress(total_pixels, total_pixels);
	std::cout << std::endl;

	// --- 【主线程】等待 Worker 收尾 ---

	thread_pool.Wait();  // pending_task_count 归零：队列空且最后一个 run() 也结束

	// --- 【主线程】写文件 ---

	film.Save("Output.ppm");

	return 0;
}
