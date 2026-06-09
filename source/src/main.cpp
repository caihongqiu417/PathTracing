#include <iostream>

#include <glm/glm.hpp>

#include "film.h"
#include"camera.h"
#include"sphere.h"
#include "thread_pool.h"

// =============================================================================
// 程序入口
// =============================================================================

int main()
{
	// --- 初始化 ---

	ThreadPool thread_pool{};

	Film film{ 1920, 1080 };
	Camera camera{ film, { 0, 0, 1 }, { 0, 0, 0 }, 45.f };

	Sphere sphere{ {0,0,0},.5f };

	glm::vec3 light_pos{ 1,1,1 };

	// --- 并行渲染 ---

	thread_pool.ParallerlFor(film.GetWidth(), film.GetHeight(), [&](size_t x, size_t y)
		{
			//对于每个像素点，从相机位置生成光线
			Ray ray = camera.GenerateRay({ x,y });

			//测试是否生成交点
			auto result = sphere.Intersect(ray);

			//如果有交点
			if (result.has_value())
			{
				//射线和球的交点（shadingpoint)
				glm::vec3 hit_point = ray.Hit(result.value());
				//shadingpoint的法线
				glm::vec3 normal = glm::normalize(hit_point - sphere.center);
				//光线垂直照射时最亮，平行时最暗
				auto L = glm::normalize(light_pos - hit_point);
				float cosine = glm::max(0.f, glm::dot(normal, L));
				film.SetPixel(x, y, { cosine,cosine,cosine });

			}

		});

	// 等 worker 线程把全部像素画完，再保存；否则 Save 时胶片可能未渲染完
	thread_pool.Wait();

	// --- 输出 ---

	film.Save("Test1.ppm");

	return 0;
}
