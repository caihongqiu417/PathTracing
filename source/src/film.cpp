#include "film.h"
#include <fstream>

Film::Film(size_t width, size_t height) : width(width), height(height)
{
	pixels.resize(width * height);
}

void Film::Save(const std::filesystem::path& filename)
{
	std::ofstream file(filename,std::ios::binary);

	file << "P6\n" << width << ' ' << height << "\n255\n";

	for (size_t y = 0; y < height; y++)
	{
		for (size_t x = 0; x < width; x++)
		{
    		// 获取像素颜色（32位浮点数，范围 0.0~1.0）
    		const glm::vec3& color = GetPixel(x, y);

    		// 转换到 0~255 并截断（32位整数）
    		glm::ivec3 color_i = glm::clamp(color * 255.f, 0.f, 255.f);

    		// 转为8位整数（0-255）写入文件
    	 	file.put(static_cast<uint8_t>(color_i.x));
            file.put(static_cast<uint8_t>(color_i.y));
            file.put(static_cast<uint8_t>(color_i.z));
		}
	}
}
