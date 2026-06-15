#include "scene.h"
#include "ray.h"

#include <glm/ext/matrix_transform.hpp>

// 把物体放进场景，并记下模型空间和世界空间怎么互转

void Scene::AddShape(const Shape& shape, std::shared_ptr<Material> material, const glm::vec3& pos, const glm::vec3& scale, const glm::vec3& rotate)
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
	instances.back().UpdateBounds();  // 计算世界空间包围盒，Build BVH 时用
}



std::optional<HitInfo> Scene::Intersect(const Ray& ray, float t_min, float t_max) const
{
	return scene_bvh.Intersect(ray, t_min, t_max);
}
