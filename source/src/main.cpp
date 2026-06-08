#include <iostream>

#include <glm/glm.hpp>

#include "film.h"
#include "thread_pool.h"

// =============================================================================
// 程序入口
// =============================================================================

int main()
{
	// --- 初始化 ---

	ThreadPool thread_pool{};
	Film film{ 1920, 1080 };

	// --- 并行渲染 ---

	thread_pool.ParallerlFor(1920, 1080, [&](size_t x, size_t y)
		{
			film.SetPixel(x, y, { 0.5f, 0.7f, 0.2f });
		});

	// --- 输出 ---

	film.Save("Test1.ppm");

	return 0;
}
