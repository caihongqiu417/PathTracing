#include "renderer/base_renderer.h"

#include <algorithm>

#include "thread/progress.h"

void BaseRenderer::Render(size_t spp, const std::filesystem::path& filename)
{
	// 图像信息
	Film& film = camera.GetFilm();
	const size_t width = film.GetWidth();
	const size_t height = film.GetHeight();
	const size_t total_pixels = width * height;
	const size_t total_samples = total_pixels * spp;
	Progress progress(total_samples);
	size_t samples_done = 0;
	size_t current_spp = 0;
	size_t increase = 1;

	// 先写一张黑图，避免看到上一次运行留下的旧结果。
	film.Save(filename);

	// 逐轮渲染
	while (current_spp < spp)
	{
		// 本轮新增 1,1,2,4,8,16,32... 个样本。
		const size_t round_samples = (std::min)(increase, spp - current_spp);

		for (size_t local_sample = 0; local_sample < round_samples; ++local_sample)
		{
			const size_t sample_index = current_spp + local_sample + 1;

			for (size_t y = 0; y < height; ++y)
			{
				for (size_t x = 0; x < width; ++x)
				{
					const glm::ivec2 pixel_coord{
						static_cast<int>(x),
						static_cast<int>(y)
					};

					// 每个样本用不同种子。
					const size_t seed = (sample_index - 1) * total_pixels + y * width + x;
					rng = Rng(static_cast<std::uint32_t>(23451334u + seed));
					film.AddSample(x, y, RenderPixel(pixel_coord));

					// 按样本更新进度。
					++samples_done;
					progress.Print(samples_done, RendererName(), sample_index, spp);
				}
			}
		}

		// 本轮结束后立刻保存一次。
		current_spp += round_samples;
		film.Save(filename);

		// 下一轮继续翻倍，最多 32。
		increase = (std::min)(current_spp, static_cast<size_t>(32));
	}

	progress.Finish(RendererName(), spp);
}