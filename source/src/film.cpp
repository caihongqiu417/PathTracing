#include "film.h"
#include"rgb.h"
#include <fstream>

// =============================================================================
// 构造
// =============================================================================

Film::Film(size_t width, size_t height)
	: width(width), height(height)
{
	pixels.resize(width * height);
}

// =============================================================================
// 文件 I/O
// =============================================================================

void Film::Save(const std::filesystem::path& filename)
{
	std::ofstream file(filename, std::ios::binary);

	// 写入 PPM 文件头（P6 = 二进制 RGB，每通道 8 位）
	file << "P6\n" << width << ' ' << height << "\n255\n";

	// 逐像素写入 RGB 字节
	for (size_t y = 0; y < height; y++)
	{
		for (size_t x = 0; x < width; x++)
		{
			//保存时把物理光照强度转换成RGB颜色以显示
			RGB rgb(GetPixel(x, y));

			file.put(static_cast<uint8_t>(rgb.r));
			file.put(static_cast<uint8_t>(rgb.g));
			file.put(static_cast<uint8_t>(rgb.b));
		}
	}
}
