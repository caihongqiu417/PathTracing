#include "bounds.h"

// 判断射线是否与盒子相交
// 思路：盒子 = X范围 ∩ Y范围 ∩ Z范围
bool Bounds::HasIntersection(const Ray& ray, float t_min, float t_max) const
{
    // 第1步：分别计算射线与三个方向上的两个平面的交点
    // t1：与左平面、下平面、后平面的交点
    // t2：与右平面、上平面、前平面的交点
    glm::vec3 t1 = (b_min - ray.origin) / ray.direction;
    glm::vec3 t2 = (b_max - ray.origin) / ray.direction;

    // 第2步：每个方向上的进入时间和离开时间
    // tmin = 进入时间（先碰到的那个平面）
    // tmax = 离开时间（后碰到的那个平面）
    glm::vec3 tmin = glm::min(t1, t2);
    glm::vec3 tmax = glm::max(t1, t2);

    // 第3步：三个方向取交集
    // 真正进入 = 三个方向都进入的时刻（取最大值）
    float t_enter = glm::max(tmin.x, glm::max(tmin.y, tmin.z));
    // 真正离开 = 任一方向离开的时刻（取最小值）
    float t_exit = glm::min(tmax.x, glm::min(tmax.y, tmax.z));

    // 第4步：判断是否有交集
    // 条件1：进入时间 <= 离开时间（确实有重叠）
    // 条件2：离开时间 >= t_min（交点在射线有效距离内）
    // 条件3：进入时间 <= t_max（交点在射线有效距离内）
    return t_enter <= t_exit && t_exit >= t_min && t_enter <= t_max;
}

bool Bounds::HasIntersection(const Ray& ray, const glm::vec3& inv_direction, float t_min, float t_max) const
{
    // 第1步：分别计算射线与三个方向上的两个平面的交点
    // t1：与左平面、下平面、后平面的交点
    // t2：与右平面、上平面、前平面的交点
    glm::vec3 t1 = (b_min - ray.origin) * inv_direction;
    glm::vec3 t2 = (b_max - ray.origin) * inv_direction;

    // 第2步：每个方向上的进入时间和离开时间
    // tmin = 进入时间（先碰到的那个平面）
    // tmax = 离开时间（后碰到的那个平面）
    glm::vec3 tmin = glm::min(t1, t2);
    glm::vec3 tmax = glm::max(t1, t2);

    // 第3步：三个方向取交集
    // 真正进入 = 三个方向都进入的时刻（取最大值）
    float t_enter = glm::max(tmin.x, glm::max(tmin.y, tmin.z));
    // 真正离开 = 任一方向离开的时刻（取最小值）
    float t_exit = glm::min(tmax.x, glm::min(tmax.y, tmax.z));

    // 第4步：判断是否有交集
    // 条件1：进入时间 <= 离开时间（确实有重叠）
    // 条件2：离开时间 >= t_min（交点在射线有效距离内）
    // 条件3：进入时间 <= t_max（交点在射线有效距离内）
    return t_enter <= t_exit && t_exit >= t_min && t_enter <= t_max;
}
