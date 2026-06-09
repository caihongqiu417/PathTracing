#include "model.h"
#include<fstream>
#include<sstream>
#include<iostream>

Model::Model(const std::filesystem::path& filename)
{
    // OBJ 里的顶点先保存在模型空间里
    // 只有放进 Scene 之后，才会出现在世界空间里的某个位置
    // 顶点位置：v x y z
    std::vector<glm::vec3> positions;
    
    // 顶点法线：vn x y z
    std::vector<glm::vec3> normals;
    
    // 打开 OBJ 文件
    std::ifstream file{ filename };
    if (!file.good())
    {
        std::cout << "打开文件失败！" << std::endl;
        return;
    }
    
    std::string line;
    char trash;            // 用来跳过 v、f、/ 这些字符
    
    // 一行一行读取
    while (std::getline(file, line))
    {
        // 转成流，方便按空格读取
        std::istringstream iss(line);
        
        // 顶点位置
        if (line.compare(0, 2, "v ") == 0)
        {
            glm::vec3 position;
            // 跳过开头的 v，再读 3 个坐标
            iss >> trash >> position.x >> position.y >> position.z;
            positions.push_back(position);
        }
        // 顶点法线
        else if(line.compare(0, 3, "vn ") == 0)
        {
            glm::vec3 normal;
            // 跳过 vn，再读 3 个分量
            iss >> trash >> trash >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        }
        // 三角形面
        else if (line.compare(0, 2, "f ") == 0)
        {
            glm::ivec3 idx_v;
            glm::ivec3 idx_vn;
            
            // 形如：f v1//vn1 v2//vn2 v3//vn3
            iss >> trash;
            
            // 读 3 个顶点下标和 3 个法线下标
            iss >> idx_v.x >> trash >> trash >> idx_vn.x;
            iss >> idx_v.y >> trash >> trash >> idx_vn.y;
            iss >> idx_v.z >> trash >> trash >> idx_vn.z;
            
            // OBJ 下标从 1 开始，所以后面要减 1
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
        // 其他内容先忽略
    }
}

// 遍历所有三角形，取最近的交点
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
