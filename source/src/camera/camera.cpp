#include "camera.h"
#include<glm/ext/matrix_clip_space.hpp>
#include<glm/ext/matrix_transform.hpp>

Camera::Camera(Film& film, const glm::vec3& pos, const glm::vec3& viewpoint, float fovy)
	:film(film),pos(pos)
{
	// 用来把屏幕上的点还原到相机前方
	camera_from_clip = glm::inverse(glm::perspective(
    	glm::radians(fovy),
    	static_cast<float>(film.GetWidth()) /static_cast<float>(film.GetHeight()) ,
   		 1.f,
    	 2.f
	));

	// 用来把相机前方的点变到世界空间里
	world_from_camera = glm::inverse(glm::lookAt(
   	 	pos,
    	viewpoint,
    	glm::vec3(0, 1, 0)
	));
}

Ray Camera::GenerateRay(const glm::ivec2& pixel_coord, const glm::vec2& offset) const
{
	// 像素坐标先换到 [-1, 1]

    glm::vec2 ndc = (glm::vec2(pixel_coord) + offset) / glm::vec2(film.GetWidth(), film.GetHeight());
    ndc.y = 1.f - ndc.y;
    ndc = 2.f * ndc - 1.f;

	// 再把这个点变到世界空间里，射线也从这里开始用世界空间表示

    glm::vec4 clip{ ndc, 0, 1 };
    glm::vec4 world_h = world_from_camera * camera_from_clip * clip;
    world_h /= world_h.w;
    glm::vec3 world = glm::vec3(world_h);

    return Ray{
        pos,                        // 射线起点：相机位置
        glm::normalize(world - pos) // 射线方向：从相机指向这个像素
    };
}
