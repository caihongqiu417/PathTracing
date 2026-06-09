#pragma once

#include"film.h"
#include"ray.h"

class Camera
{
public:
	// pos 是相机位置，viewpoint 是看向的位置，fovy 是垂直视野角
	Camera(Film& film, const glm::vec3& pos, const glm::vec3& viewpoint, float fovy);

	// 给定一个像素，生成穿过这个像素的射线
	Ray GenerateRay(const glm::ivec2& pixel_coord, const glm::vec2& offset = { 0.5,0.5 }) const;

	Film& GetFilm() { return film; }
	const Film& GetFilm()const { return film; }
private:
	Film& film;
	glm::vec3 pos;
	glm::mat4 camera_from_clip;
	glm::mat4 world_from_camera;
};