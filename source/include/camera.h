#pragma once

#include"film.h"
#include"ray.h"

class Camera
{
public:
	/// @brief 构造函数
	/// @param film 胶片
	/// @param pos 相机在哪
	/// @param viewpoint 相机看向哪
	/// @param fovy 垂直视野角(度)
	Camera(Film& film, const glm::vec3& pos, const glm::vec3& viewpoint, float fovy);

	/// @brief 生成射线（相机->屏幕像素）
	/// @param pixel_coord 像素坐标
	/// @param offset 像素内偏移，默认中心(0.5,0.5)
	/// @return 射线
	Ray GenerateRay(const glm::ivec2& pixel_coord, const glm::vec2& offset = { 0.5,0.5 }) const;

	Film& GetFilm() { return film; }
	const Film& GetFilm()const { return film; }
private:
	Film& film;
	glm::vec3 pos;
	glm::mat4 camera_from_clip;
	glm::mat4 world_from_camera;
};