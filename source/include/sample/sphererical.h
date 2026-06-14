#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

//在圆盘采样
inline glm::vec2 UniformSampleUnitDisk(const glm::vec2& u)
{
	float r = glm::sqrt(u.x);	//弥补面积误差
    float theta = 2.f * glm::pi<float>() * u.y;
    return { r * glm::cos(theta), r * glm::sin(theta) };
};

// 余弦重要性采样：在局部半球上采样，靠近法线（y轴）的方向概率更高
// pdf = cos(θ) / π
inline glm::vec3 CosineSampleHemisphere(const glm::vec2& u)
{
    // 第一步：在单位圆盘上均匀取一个点 (x, z)
    glm::vec2 disk = UniformSampleUnitDisk(u);

    // 第二步：把圆盘上的点向上投影到半球面
    // 圆盘点到球面的投影：y = √(1 - x² - z²)
    float y = glm::sqrt(glm::max(0.f, 1.f - disk.x * disk.x - disk.y * disk.y));

    return glm::normalize(glm::vec3(disk.x, y, disk.y));
}