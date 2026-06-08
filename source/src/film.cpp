#include "film.h"

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
			const glm::vec3& color = GetPixel(x, y);

			// 浮点 [0, 1] → 整数 [0, 255]
			glm::ivec3 color_i = glm::clamp(color * 255.f, 0.f, 255.f);

			file.put(static_cast<uint8_t>(color_i.x));
			file.put(static_cast<uint8_t>(color_i.y));
			file.put(static_cast<uint8_t>(color_i.z));
		}
	}
}
