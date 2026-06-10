#include "model.h"
#include <rapidobj/rapidobj.hpp>
#include <iostream>

// 加载 OBJ
Model::Model(const std::filesystem::path& filename)
{
    auto result = rapidobj::ParseFile(filename);

    if (result.error)
    {
        std::cout << "打开文件失败！" << std::endl;
        return;
    }

    rapidobj::Triangulate(result);

    const auto& positions = result.attributes.positions;
    const auto& normals = result.attributes.normals;

    for (const auto& shape : result.shapes)
    {
        const auto& indices = shape.mesh.indices;
        for (size_t i = 0; i < indices.size(); i += 3)
        {
            glm::vec3 p0{ positions[indices[i].position_index * 3],
                         positions[indices[i].position_index * 3 + 1],
                         positions[indices[i].position_index * 3 + 2] };
            glm::vec3 p1{ positions[indices[i + 1].position_index * 3],
                         positions[indices[i + 1].position_index * 3 + 1],
                         positions[indices[i + 1].position_index * 3 + 2] };
            glm::vec3 p2{ positions[indices[i + 2].position_index * 3],
                         positions[indices[i + 2].position_index * 3 + 1],
                         positions[indices[i + 2].position_index * 3 + 2] };

            glm::vec3 n0{ normals[indices[i].normal_index * 3],
                         normals[indices[i].normal_index * 3 + 1],
                         normals[indices[i].normal_index * 3 + 2] };
            glm::vec3 n1{ normals[indices[i + 1].normal_index * 3],
                         normals[indices[i + 1].normal_index * 3 + 1],
                         normals[indices[i + 1].normal_index * 3 + 2] };
            glm::vec3 n2{ normals[indices[i + 2].normal_index * 3],
                         normals[indices[i + 2].normal_index * 3 + 1],
                         normals[indices[i + 2].normal_index * 3 + 2] };

            triangles.push_back(Triangle(p0, p1, p2, n0, n1, n2));
        }
    }

    Build();
}

// 取最近的交点
std::optional<HitInfo> Model::Intersect(const Ray& ray, float t_min, float t_max) const
{
    //和AABB相交测试
    if (!bounds.HasIntersection(ray, t_min, t_max))
        return{};

    //交点信息
    std::optional<HitInfo> closest;
	float closest_t = t_max;

	for (const Triangle& tri : triangles)
	{
		std::optional<HitInfo> hit = tri.Intersect(ray, t_min, closest_t);
		if (hit.has_value())
		{
			closest = hit;
			closest_t = hit->t;
		}
	}

	return closest;
}

void Model::Build()
{
    //遍历所有三角形
    for (const auto& triangle : triangles)
    {
        //让包围盒包括三角形
        bounds.Expand(triangle.p0);
        bounds.Expand(triangle.p1);
        bounds.Expand(triangle.p2);
    }
}
