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

	void Print(std::size_t done) const
	{
		const float ratio = total == 0
			? 1.f
			: static_cast<float>(done) / static_cast<float>(total);
		const int filled = static_cast<int>(ratio * static_cast<float>(bar_width));

		// 用一行字符实时覆盖终端里的旧进度。
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
		std::cout << "] " << static_cast<int>(ratio * 100.f) << "%" << std::flush;
	}

	void Finish() const
	{
		Print(total);
		// 最后补一个换行，避免后面的输出接在进度条后面。
		std::cout << std::endl;
	}

private:
	std::size_t total;
	int bar_width;
};
