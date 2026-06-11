#pragma once

#include <glm/glm.hpp>
#include <array>

/// @brief sRGB整数值（int 0-255）
class RGB {
public:
    // 前向声明
    friend inline RGB Lerp(const RGB& a, const RGB& b, float t);

    /// @brief 生成热力图颜色
    /// @param t 温度值，范围 [0, 1]，0=最低温(紫色)，1=最高温(黄色)
    /// @return 对应的RGB颜色
    inline static RGB GenerateHeatmapRGB(float t)
    {
        // 25个颜色的调色板（紫 -> 蓝 -> 青 -> 绿 -> 黄）
        static const std::array<RGB, 25> color_pallet
        {
            RGB{68, 1, 84},   // 紫
            RGB{71, 17, 100},
            RGB{72, 31, 112},
            RGB{71, 45, 123},
            RGB{68, 58, 131},
            RGB{64, 70, 136},
            RGB{59, 82, 139},
            RGB{54, 93, 141},
            RGB{49, 104, 142},
            RGB{44, 114, 142},
            RGB{40, 124, 142},
            RGB{36, 134, 142},
            RGB{33, 144, 140}, // 青
            RGB{31, 154, 138},
            RGB{32, 164, 134},
            RGB{39, 173, 129},
            RGB{53, 183, 121},
            RGB{71, 193, 110},
            RGB{93, 200, 99},
            RGB{117, 208, 84},
            RGB{143, 215, 68},
            RGB{170, 220, 50},
            RGB{199, 224, 32},
            RGB{227, 228, 24},
            RGB{253, 231, 37}  // 黄
        };

        if (t <= 0.0f) return color_pallet[0];
        if (t >= 1.0f) return color_pallet.back();

        float idx_float = t * (color_pallet.size() - 1);  // 映射到 [0, 24]
        size_t idx = (size_t)glm::floor(idx_float);       // 左索引
        float frac = glm::fract(idx_float);               // 混合比例

        return Lerp(color_pallet[idx], color_pallet[idx + 1], frac);
    }

    // 直接设置sRGB整数值（0-255）
    RGB(int r, int g, int b) : r(r), g(g), b(b) {}
    
    // 从物理真实光照转换而来，应用伽马压缩存储
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

/// @brief 在两个RGB颜色之间线性插值
/// @param a 起始颜色
/// @param b 终止颜色
/// @param t 插值参数（0.0=a, 1.0=b, 0.5=中间色）
/// @return 插值结果颜色
inline RGB Lerp(const RGB& a, const RGB& b, float t)
{
    return RGB{
        glm::clamp<int>(a.r + (b.r - a.r) * t, 0, 255),
        glm::clamp<int>(a.g + (b.g - a.g) * t, 0, 255),
        glm::clamp<int>(a.b + (b.b - a.b) * t, 0, 255)
    };
}