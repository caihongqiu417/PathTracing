#include "scene.h"
#include "ray.h"

#include <glm/ext/matrix_transform.hpp>

// 把物体放进场景，并记下模型空间和世界空间怎么互转

void Scene::AddShape(const Shape& shape, const Material& material,const glm::vec3& pos, const glm::vec3& scale, const glm::vec3& rotate)
{
	// 模型里的点先在模型空间里
	// 这个矩阵负责把它们放到世界空间里
	glm::mat4 world_from_object =
		glm::translate(glm::mat4(1.f), pos) *
		glm::rotate(glm::mat4(1.f), glm::radians(rotate.z), glm::vec3(0, 0, 1)) *
		glm::rotate(glm::mat4(1.f), glm::radians(rotate.y), glm::vec3(0, 1, 0)) *
		glm::rotate(glm::mat4(1.f), glm::radians(rotate.x), glm::vec3(1, 0, 0)) *
		glm::scale(glm::mat4(1.f), scale);

	instances.push_back(ShapeInstance{
		shape,
		material,
		world_from_object,
		glm::inverse(world_from_object)  // 反过来：世界空间 -> 模型空间
	});
}

// 求交时，对场景里的每个物体都重复下面这几步：
// 1. 先把世界空间里的射线变到这个物体的模型空间里
// 2. 在模型空间里做求交
// 3. 再把交点和法线变回世界空间

std::optional<HitInfo> Scene::Intersect(const Ray& ray, float t_min, float t_max) const
{
	std::optional<HitInfo> closest;
	float closest_t = t_max;

	for (const auto& instance : instances)
	{
		// 先把射线变到当前物体的模型空间里
		Ray ray_object = ray.WorldToObject(instance.object_from_world);

		// 在模型空间里做求交
		std::optional<HitInfo> hit = instance.shape.Intersect(ray_object, t_min, closest_t);
		if (!hit.has_value())
			continue;

		// 交点变回世界空间
		glm::vec3 world_point = glm::vec3(instance.world_from_object * glm::vec4(hit->point, 1.f));

		// 统一回到世界空间后，再比较哪个交点更近
		float world_t = glm::dot(world_point - ray.origin, ray.direction);
		if (world_t <= t_min || world_t >= closest_t)
			continue;

		// 法线不能直接跟点一样去变，要保证它变完后仍然垂直于表面
		glm::vec3 world_normal = glm::normalize(glm::vec3(
			glm::transpose(instance.object_from_world) * glm::vec4(hit->normal, 0.f)));

		hit->t = world_t;
		hit->point = world_point;
		hit->normal = world_normal;
		hit->material = &instance.material;
		closest = hit;
		closest_t = world_t;
	}

	return closest;
}
