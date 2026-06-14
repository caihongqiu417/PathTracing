#include "renderer/base_renderer.h"

#include <algorithm>
#include <atomic>

#include "thread/progress.h"
#include "thread/thread_pool.h"
#include "thread/spin_lock.h"

// 每个线程独立保存自己的 rng，互不干扰
thread_local Rng BaseRenderer::rng(0);

void BaseRenderer::Render(size_t spp, const std::filesystem::path& filename)
{
	Film& film = camera.GetFilm();
	const size_t width        = film.GetWidth();
	const size_t height       = film.GetHeight();
	const size_t total_pixels = width * height;
	const size_t total_samples = total_pixels * spp;

	Progress progress(total_samples);
	std::atomic<size_t> samples_done{ 0 };
	SpinLock progress_lock;

	size_t current_spp = 0;
	size_t increase    = 1;

	film.Save(filename);

	while (current_spp < spp)
	{
		const size_t round_samples = (std::min)(increase, spp - current_spp);

		for (size_t local_sample = 0; local_sample < round_samples; ++local_sample)
		{
			const size_t sample_index = current_spp + local_sample + 1;

			thread_pool.ParallerlFor(width, height, [&](size_t x, size_t y)
			{
				// 每个像素用确定性种子，保证不同线程不冲突
				const size_t seed = (sample_index - 1) * total_pixels + y * width + x;
				rng = Rng(static_cast<std::uint32_t>(23451334u + seed));

				// AddSample 写不同像素的内存
				film.AddSample(x, y, RenderPixel({ (int)x, (int)y }));

				// 进度条加锁保护
				const size_t done = ++samples_done;
				Guard guard(progress_lock);
				progress.Print(done, RendererName(), sample_index, spp);

			}, 2.0f); 

			thread_pool.Wait();  // 等本轮所有像素跑完，再保存图片
		}

		current_spp += round_samples;
		film.Save(filename);
		increase = (std::min)(current_spp, static_cast<size_t>(32));
	}

	progress.Finish(RendererName(), spp);
}
