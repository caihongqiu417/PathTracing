#pragma once
#include<filesystem>
#include<vector>
#include<glm/glm.hpp>

/*
* 胶片类：相当于FrameBuffer,用于存储最终的颜色
*/
class Film
{
public:
	Film(size_t width, size_t height);

	//保存到文件（PPM)
	void Save(const std::filesystem::path& filename);

	//获取宽、高
	inline size_t GetWidth()const { return width; }
	inline size_t GetHeight()const { return height; }


	//获取颜色
	glm::vec3 GetPixel(size_t x, size_t y) { return pixels[y * width + x]; }
	void SetPixel(size_t x, size_t y, const glm::vec3& color) { pixels[y * width + x] = color; }

private:
	size_t width, height;
	std::vector<glm::vec3> pixels;
};