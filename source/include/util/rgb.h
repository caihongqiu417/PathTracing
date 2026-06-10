#pragma once

#include <glm/glm.hpp>

class RGB {
public:
    // 直接设置sRGB整数值（0-255）
    RGB(int r, int g, int b) : r(r), g(g), b(b) {}
    
    // 从物理真实光照（线性空间）转换而来，应用伽马压缩存储
    RGB(const glm::vec3 &color) {
        r = glm::clamp<int>(glm::pow(color.x, 1.0f / 2.2f) * 255, 0, 255);
        g = glm::clamp<int>(glm::pow(color.y, 1.0f / 2.2f) * 255, 0, 255);
        b = glm::clamp<int>(glm::pow(color.z, 1.0f / 2.2f) * 255, 0, 255);
    }
    
    // 转换为物理真实光照（线性空间），用于光照计算
    operator glm::vec3() const {
        return glm::vec3(
            glm::pow(r / 255.0f, 2.2f),
            glm::pow(g / 255.0f, 2.2f),
            glm::pow(b / 255.0f, 2.2f)
        );
    }
    
public:
    int r, g, b;  // sRGB整数值（0-255）
};