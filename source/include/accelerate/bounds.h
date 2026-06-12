#pragma once

#include"camera/ray.h"
#include<glm/glm.hpp>

// 轴对齐包围盒AABB
// 由最小点(b_min)和最大点(b_max)定义的一个长方体
struct Bounds
{
	Bounds() 
        : b_min(glm::vec3(0)), b_max(glm::vec3(0)) {
    }
    
	// 构造函数：传入包围盒的两个对角顶点
    // b_min - 最小点
    // b_max - 最大点
    Bounds(const glm::vec3& b_min, const glm::vec3& b_max)
        : b_min(b_min), b_max(b_max) {
    }

    // 扩展包围盒，使其包含新点
    void Expand(const glm::vec3& pos)
    {
        b_min = glm::min(pos, b_min);  // 更新最小边界
        b_max = glm::max(pos, b_max);  // 更新最大边界
    }

    //获取表面积
    float Area()const {
        auto diag = Diagonal();
        return (diag.x * (diag.y + diag.z) + diag.y * diag.z) * 2.f;
    }

    // 判断射线是否与包围盒相交
    // ray   - 射线
    // t_min - 最小检测距离
    // t_max - 最大检测距离
    bool HasIntersection(const Ray& ray, float t_min, float t_max) const;

    //优化：除法=>乘倒数
    bool HasIntersection(const Ray& ray, const glm::vec3& inv_direction/*光线方向的倒数*/, float t_min, float t_max)const;

    //获取包围盒的对角线
    glm::vec3 Diagonal()const { return b_max - b_min; }

    glm::vec3 b_min;  // 最小点
    glm::vec3 b_max;  // 最大点
};