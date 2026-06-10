#pragma once

#include <filesystem>
#include <vector>
#include <glm/glm.hpp>

// 一个像素
struct Pixel
{
	glm::vec3 color{ 0,0,0 };
	int sample_count{ 0 };
};

// 胶片
class Film
{
public:
	// 图像大小
	Film(size_t width, size_t height);

	// 保存成 PPM
	void Save(const std::filesystem::path& filename);

	size_t GetWidth() const { return width; }
	size_t GetHeight() const { return height; }

	// 读一个像素
	Pixel GetPixel(size_t x, size_t y)
	{
		return pixels[y * width + x];
	}

	// 给像素累加一个样本
	void AddSample(size_t x, size_t y, const glm::vec3& color)
	{
		pixels[y * width + x].color += color;
		pixels[y * width + x].sample_count++;
	}

private:
	size_t width, height;

	// 按行存像素
	std::vector<Pixel> pixels;
};
