#include "film.h"
#include"rgb.h"
#include"thread/thread_pool.h"
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
    file << "P6\n" << width << ' ' << height << "\n255\n";

    // 预分配缓冲区（RGB 三个通道）
    std::vector<uint8_t> buffer(width * height * 3);

    // 并行处理所有像素
    thread_pool.ParallerlFor(width, height, [&](size_t x, size_t y)
    {
        auto pixel = GetPixel(x, y);

        // 没有样本就先写黑色
        RGB rgb = pixel.sample_count == 0
            ? RGB(glm::vec3{0.f, 0.f, 0.f})
            : RGB(pixel.color / static_cast<float>(pixel.sample_count));

        // 计算在缓冲区中的位置（每个像素占3字节）
        size_t idx = (y * width + x) * 3;
        buffer[idx]     = static_cast<uint8_t>(rgb.r);
        buffer[idx + 1] = static_cast<uint8_t>(rgb.g);
        buffer[idx + 2] = static_cast<uint8_t>(rgb.b);
    });

    // 等待所有任务完成
    thread_pool.Wait();

    // 一次性写入文件
    file.write(reinterpret_cast<char*>(buffer.data()), buffer.size());
}