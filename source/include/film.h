#pragma once

#include <filesystem>
#include <vector>
#include <glm/glm.hpp>

// 存放整张图的像素颜色，并支持保存成 PPM
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
	glm::vec3 GetPixel(size_t x, size_t y) { return pixels[y * width + x]; }
	void SetPixel(size_t x, size_t y, const glm::vec3& color) { pixels[y * width + x] = color; }

private:
	size_t width, height;
	std::vector<glm::vec3> pixels;  // 按行存，索引 = y * width + x
};
