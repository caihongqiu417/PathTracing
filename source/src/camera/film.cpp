#include "film.h"
#include"rgb.h"
#include <fstream>

// 构造
Film::Film(size_t width, size_t height)
	: width(width), height(height)
{
	pixels.resize(width * height);
}

// 保存图片
void Film::Save(const std::filesystem::path& filename)
{
	std::ofstream file(filename, std::ios::binary);

	// 写文件头
	file << "P6\n" << width << ' ' << height << "\n255\n";

	// 写每个像素
	for (size_t y = 0; y < height; y++)
	{
	   for (size_t x = 0; x < width; x++)
	   {
	       auto pixel = GetPixel(x, y);

	       // 没有样本就先写黑色。
	       RGB rgb = pixel.sample_count == 0
	       	? RGB(glm::vec3{ 0.f, 0.f, 0.f })
	       	: RGB(pixel.color / static_cast<float>(pixel.sample_count));

	       // 写 RGB
	       file.put(static_cast<uint8_t>(rgb.r));
	       file.put(static_cast<uint8_t>(rgb.g));
	       file.put(static_cast<uint8_t>(rgb.b));
	   }
	}
}
