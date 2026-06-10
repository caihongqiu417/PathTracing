#include "model.h"
#include<fstream>
#include<iostream>
#include<sstream>

// 加载 OBJ
Model::Model(const std::filesystem::path& filename)
{
    // 顶点和法线先存在模型空间里。
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;

    // 打开文件
    std::ifstream file{ filename };
    if (!file.good())
    {
        std::cout << "打开文件失败！" << std::endl;
        return;
    }

    std::string line;
    char trash;  // 用来跳过 v、f、/

    // 逐行解析
    while (std::getline(file, line))
    {
        std::istringstream iss(line);

        // 顶点
        if (line.compare(0, 2, "v ") == 0)
        {
            glm::vec3 position;
            iss >> trash >> position.x >> position.y >> position.z;
            positions.push_back(position);
        }
        // 法线
        else if (line.compare(0, 3, "vn ") == 0)
        {
            glm::vec3 normal;
            iss >> trash >> trash >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        }
        // 三角形
        else if (line.compare(0, 2, "f ") == 0)
        {
            glm::ivec3 idx_v;
            glm::ivec3 idx_vn;

            iss >> trash;

            iss >> idx_v.x >> trash >> trash >> idx_vn.x;
            iss >> idx_v.y >> trash >> trash >> idx_vn.y;
            iss >> idx_v.z >> trash >> trash >> idx_vn.z;

            // OBJ 下标从 1 开始
            if (idx_v.x <= positions.size() && idx_v.y <= positions.size() && idx_v.z <= positions.size() &&
                idx_vn.x <= normals.size() && idx_vn.y <= normals.size() && idx_vn.z <= normals.size())
            {
                triangles.push_back(Triangle(
                    positions[idx_v.x - 1], 
                    positions[idx_v.y - 1], 
                    positions[idx_v.z - 1],
                    normals[idx_vn.x - 1], 
                    normals[idx_vn.y - 1], 
                    normals[idx_vn.z - 1]
                ));
            }
        }
    }
}

// 取最近的交点
std::optional<HitInfo> Model::Intersect(const Ray& ray, float t_min, float t_max) const
{
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
