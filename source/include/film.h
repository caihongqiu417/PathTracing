#pragma once

#include <filesystem>
#include <vector>
#include <glm/glm.hpp>

// =============================================================================
// Film — 图像缓冲区（FrameBuffer）
// 按像素存储 RGB 颜色，支持导出为 PPM 文件
// =============================================================================
class Film
{
public:
	// --- 构造 ---

	/// @param width  图像宽度（像素）
	/// @param height 图像高度（像素）
	Film(size_t width, size_t height);

	// --- 文件 I/O ---

	/// 保存为二进制 PPM（P6 格式）
	void Save(const std::filesystem::path& filename);

	// --- 尺寸查询 ---

	size_t GetWidth() const { return width; }
	size_t GetHeight() const { return height; }

	// --- 像素读写（color 分量范围 [0, 1]）---

	glm::vec3 GetPixel(size_t x, size_t y) { return pixels[y * width + x]; }
	void SetPixel(size_t x, size_t y, const glm::vec3& color) { pixels[y * width + x] = color; }

private:
	// --- 成员变量 ---

	size_t width, height;
	std::vector<glm::vec3> pixels;  // 行优先，索引 = y * width + x
};
