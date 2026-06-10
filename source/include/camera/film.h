#pragma once

#include <filesystem>
#include <vector>
#include <glm/glm.hpp>

/// @brief 一个像素：包含总的颜色和采样数
struct Pixel {
	glm::vec3 color{ 0,0,0 };
	int sample_count{ 0 };
};

/// @brief 存放整张图的像素颜色，并支持保存成 PPM
class Film
{
public:
	// width 和 height 是图像大小
	Film(size_t width, size_t height);

	// 保存为二进制 PPM（P6）
	void Save(const std::filesystem::path& filename);

	size_t GetWidth() const { return width; }
	size_t GetHeight() const { return height; }

	// color 的每个分量都在 [0, 1]
	Pixel GetPixel(size_t x, size_t y)
	{
		return pixels[y * width + x];
	}

	// SSAA:对于每个像素[x,y]累加多次采样，最后取平均
	void AddSample(size_t x, size_t y, const glm::vec3& color)
	{
   		pixels[y * width + x].color += color;      // 累加采样颜色
    	pixels[y * width + x].sample_count++;      // 采样计数+1
	}

private:
	size_t width, height;

	//假设胶片存储的是物理意义上的光照强度
	std::vector<Pixel> pixels;  // 按行存，索引 = y * width + x
};
