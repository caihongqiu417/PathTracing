#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include "util/rng.h"

//  圆盘采样
//  u.x ∈ [0, 1]  →  控制离圆心多远（r）
// u.y ∈ [0, 1]  →  控制在哪个角度（θ）
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
    // 1. 在单位圆盘上均匀取一个点 (x, z)
    glm::vec2 disk = UniformSampleUnitDisk(u);

    // 2. 把圆盘上的点向上投影到半球面  y = √(1 - x² - z²)
    float y = glm::sqrt(glm::max(0.f, 1.f - disk.x * disk.x - disk.y * disk.y));

    return glm::normalize(glm::vec3(disk.x, y, disk.y));
}

// 均匀采样上半球
inline glm::vec3 UniformSampleHemisphere(Rng& rng) {
    glm::vec3 result;
    do {
        result = { rng.Float01(), rng.Float01(), rng.Float01() };
        result = result * 2.f - 1.f;
    } while (glm::length(result) > 1);
    if (result.y < 0) {
        result.y = -result.y;
    }
    return glm::normalize(result);
}

// 均匀采样球面
inline glm::vec3 UniformSampleSphere(Rng& rng) {
    glm::vec3 result;
    do {
        result = { rng.Float01(), rng.Float01(), rng.Float01() };
        result = result * 2.f - 1.f;
    } while (glm::length(result) > 1);
    return glm::normalize(result);
}