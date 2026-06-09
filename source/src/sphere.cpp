#include "sphere.h"

std::optional<float> Sphere::Intersect(const Ray &ray) const {
    // 相机到球心的向量（从射线起点指向球心）
    glm::vec3 co = ray.origin - center;
    
    // 解二次方程 t*2+2Bt+C=0;
    
    float b = 2 * glm::dot(ray.direction, co);  // B = 2·(dir · o-c)
    float c = glm::dot(co, co) - radius * radius; // C = |o-c|² - r²
    
    float delta = b * b - 4 * c;  // 判别式 Δ = B² - 4AC
    
    // 无实数解 → 没有交点
    if (delta < 0) { return {}; }
    
    // 取较小的根（离相机更近的交点）
    float hit_t = (-b - glm::sqrt(delta)) * 0.5;
    
    // 如果较小根为负（在相机后方），尝试较大的根
    if (hit_t < 0) {
        hit_t = (-b + glm::sqrt(delta)) * 0.5;
    }
    
    // 交点在相机前方 → 返回距离
    if (hit_t > 0) {
        return hit_t;
    }
    
    // 所有交点都在相机后方 → 无有效交点
    return {};
}