#pragma once

#include <cstddef>
#include <iostream>

// 仅由主线程使用的终端进度条。
class Progress
{
public:
	explicit Progress(std::size_t total, int bar_width = 40)
		: total(total), bar_width(bar_width)
	{
	}

	void Print(std::size_t done, const char* renderer_name, std::size_t current_sample, std::size_t total_spp) const
	{
		const float ratio = total == 0
			? 1.f
			: static_cast<float>(done) / static_cast<float>(total);
		const int percent = static_cast<int>(ratio * 100.f);

		// 百分比和样本轮次都没变就先不刷新，减少终端输出开销。
		if (done < total && percent == last_percent && current_sample == last_sample)
			return;

		last_percent = percent;
		last_sample = current_sample;
		const int filled = static_cast<int>(ratio * static_cast<float>(bar_width));

		// 用一行字符实时覆盖终端里的旧进度。
		std::cout << '\r'
			<< "Sample " << current_sample << "/" << total_spp
			<< " | " << renderer_name << " ";
		std::cout << "[";
		for (int i = 0; i < bar_width; ++i)
		{
			if (i < filled)
				std::cout << '=';
			else if (i == filled)
				std::cout << '>';
			else
				std::cout << ' ';
		}
		std::cout << "] " << percent << "%" << std::flush;
	}

	void Finish(const char* renderer_name, std::size_t total_spp) const
	{
		Print(total, renderer_name, total_spp, total_spp);
		// 最后补一个换行，避免后面的输出接在进度条后面。
		std::cout << std::endl;
	}

private:
	std::size_t total;
	int bar_width;
	mutable int last_percent = -1;
	mutable std::size_t last_sample = static_cast<std::size_t>(-1);
};
