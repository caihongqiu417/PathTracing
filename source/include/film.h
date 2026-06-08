#pragma once

#include <filesystem>
#include <vector>
#include <glm/glm.hpp>

/// 图像缓冲区，按像素存储 RGB 颜色，可导出为 PPM 文件
class Film
{
public:
	/// @param width  图像宽度（像素）
	/// @param height 图像高度（像素）
	Film(size_t width, size_t height);

	/// 将当前图像保存为二进制 PPM（P6）文件
	void Save(const std::filesystem::path& filename);

	size_t GetWidth() const { return width; }
	size_t GetHeight() const { return height; }

	/// 读取 / 写入指定坐标的颜色，color 分量范围 [0, 1]
	glm::vec3 GetPixel(size_t x, size_t y) { return pixels[y * width + x]; }
	void SetPixel(size_t x, size_t y, const glm::vec3& color) { pixels[y * width + x] = color; }

private:
	size_t width, height;
	std::vector<glm::vec3> pixels;  // 行优先存储，索引 = y * width + x
};
