#include "camera.h"
#include<glm/ext/matrix_clip_space.hpp>
#include<glm/ext/matrix_transform.hpp>

Camera::Camera(Film& film, const glm::vec3& pos, const glm::vec3& viewpoint, float fovy)
	:film(film),pos(pos)
{
	// 屏幕点 → (P⁻¹) → 相机空间
	// 计算裁剪空间到相机空间的逆投影矩阵
	camera_from_clip = glm::inverse(glm::perspective(
    	glm::radians(fovy),                                          // 垂直视野角（度转弧度）
    	static_cast<float>(film.GetWidth()) /static_cast<float>(film.GetHeight()) ,      // 纵横比 = 宽/高
   		 1.f,   // 近平面（相机能看到的最小距离）
    	 2.f  // 远平面（相机能看到的距离范围）
	));

	//相机空间 → (V⁻¹) → 世界空间
	// 计算观察矩阵的逆，将相机空间的点转换到世界空间
	world_from_camera = glm::inverse(glm::lookAt(
   	 	pos,           // 相机位置
    	viewpoint,     // 看向哪
    	glm::vec3(0, 1, 0)  // 上方向
	));
}

Ray Camera::GenerateRay(const glm::ivec2& pixel_coord, const glm::vec2& offset) const
{
    // 1. 像素坐标 → NDC [0,1]
    // (像素坐标 + 偏移) / 图像分辨率
    glm::vec2 ndc = (glm::vec2(pixel_coord) + offset) / glm::vec2(film.GetWidth(), film.GetHeight());
    
    // 2. 翻转 Y 轴（屏幕 Y 向下，NDC Y 向上）
    ndc.y = 1.f - ndc.y;
    
    // 3. NDC [0,1] → 裁剪空间 [-1,1]
    ndc = 2.f * ndc - 1.f;
    
    // 4. 构造近平面上的点（z=0, w=1）
    glm::vec4 clip{ ndc, 0, 1 };
    
    // 5. 齐次坐标 → 世界坐标（必须做透视除法）
    //    矩阵乘法得到的是 vec4 (x, y, z, w)，还不是真正的 3D 点。
    //    透视投影下 w ≠ 1，只有 (x/w, y/w, z/w) 才是世界空间里的点。
    glm::vec4 world_h = world_from_camera * camera_from_clip * clip;
    world_h /= world_h.w;
    glm::vec3 world = glm::vec3(world_h);
    
    // 6. 返回射线：从相机位置指向世界点
    return Ray{
        pos,                           // 起点：相机位置
        glm::normalize(world - pos)    // 方向：指向屏幕点对应的世界方向
    };
}
