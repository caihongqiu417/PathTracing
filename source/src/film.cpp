#include "film.h"

#include <fstream>

Film::Film(size_t width, size_t height)
	: width(width), height(height)
{
	pixels.resize(width * height);
}

void Film::Save(const std::filesystem::path& filename)
{
	std::ofstream file(filename, std::ios::binary);

	// P6：二进制 PPM，每通道 8 位
	file << "P6\n" << width << ' ' << height << "\n255\n";

	for (size_t y = 0; y < height; y++)
	{
		for (size_t x = 0; x < width; x++)
		{
			const glm::vec3& color = GetPixel(x, y);

			// [0, 1] 浮点色 → [0, 255] 整数
			glm::ivec3 color_i = glm::clamp(color * 255.f, 0.f, 255.f);

			file.put(static_cast<uint8_t>(color_i.x));
			file.put(static_cast<uint8_t>(color_i.y));
			file.put(static_cast<uint8_t>(color_i.z));
		}
	}
}
