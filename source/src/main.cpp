#include <atomic>
#include <chrono>
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
#include "progress.h"
#include "rng.h"
#include"rgb.h"
#include "thread_pool.h"

// =============================================================================
// 程序入口
// 【主线程】   刷进度 + Wait + Save
// 【入队线程】 ParallerlFor
// 【Worker】   算像素
// =============================================================================

int main()
{
	// 每条路径最多反弹 8 次，够看出全局光照，也能避免少数光线跑太久。
	constexpr int max_bounces = 8;
	// 新光线从表面上方一点点出发，避免再次打回当前表面。
	constexpr float ray_origin_bias = 1e-4f;

	// --- 【主线程】场景初始化 ---

	ThreadPool thread_pool{};

	Film film{ 1920, 1080 };
	Camera camera{ film, { -3.6, 0, 0}, { 0, 0, 0 }, 45.f };
	int spp = 16; // 每个像素采样 16 次

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
	Progress progress(total_pixels);
	std::atomic<size_t> pixels_done{ 0 };
	std::atomic<bool> enqueue_done{ false };

	// --- 【入队线程】提交任务 ---

std::thread enqueue_thread([&]()
{
    thread_pool.ParallerlFor(film.GetWidth(), film.GetHeight(), [&](size_t x, size_t y)
    {
        // ↓↓↓ 【Worker 线程】执行 ↓↓↓

        // 每个像素用自己的随机数状态，线程之间互不影响。
        Rng rng(static_cast<std::uint32_t>(23451334u + y * film.GetWidth() + x));

        // ================================================================
        // 每个像素采样 spp 次
        // ================================================================
        for (int sample = 0; sample < spp; sample++)
        {
            // 最终反照率
            glm::vec3 beta = { 1,1,1 };
            glm::vec3 color = { 0,0,0 };

            // 若 shape 是 Scene：内部会光线从世界空间转换到模型空间，然后与模型空间内的场景求交，再将交点转回世界空间
            Ray ray = camera.GenerateRay(
                { x, y },
                rng.PixelOffset() // 像素内抖动，减少锯齿和规则噪点
            );

            // 一条路径会不断反弹，直到打空或者达到上限。
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
                    
                    Frame frame(hit_info->normal); // 用法线建一个局部坐标系

                    glm::vec3 light_direction;
                    if (hit_info->material->is_specular/*1.镜面反射*/)
                    {
                        //将射线取反，然后绕y轴转180°=>得到反射光方向
                        glm::vec3 view_direction = frame.LocalFromWorld(-ray.direction);
                        light_direction = { -view_direction.x,view_direction.y,-view_direction.z };
                    }
                    else {/*2.漫反射*/
                        // 在局部半球内随机采样一个反射方向。
                        light_direction = rng.SampleHemisphere();
                    }
                    // 采样是在局部空间里做的，最后再转回世界空间。
                    ray.direction = frame.WorldFromLocal(light_direction);
                }
                else
                {
                    break;
                }
            }
            
            // 多次采样会在 Film 里做累计，保存时再取平均。
            film.AddSample(x, y, color);
        }

        pixels_done.fetch_add(1, std::memory_order_relaxed);
    });

    enqueue_done.store(true, std::memory_order_release);
});

	// --- 【主线程】刷新进度条 ---

	while (!enqueue_done.load(std::memory_order_acquire)
		|| pixels_done.load(std::memory_order_relaxed) < total_pixels)
	{
		progress.Print(pixels_done.load(std::memory_order_relaxed));
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	enqueue_thread.join();
	progress.Finish();

	thread_pool.Wait();

	film.Save("Output.ppm");

	return 0;
}
