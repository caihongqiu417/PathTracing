#include <iostream>

#include <glm/glm.hpp>

#include "film.h"
#include "thread_pool.h"

int main()
{
	ThreadPool thread_pool{};
	Film film{ 1920, 1080 };

	// 并行写入每个像素的颜色
	thread_pool.ParallerlFor(1920, 1080, [&](size_t x, size_t y)
		{
			film.SetPixel(x, y, { 0.5f, 0.7f, 0.2f });
		});

	film.Save("Test1.ppm");

	return 0;
}
