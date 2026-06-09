#include "model.h"
#include<fstream>
#include<sstream>
#include<iostream>

Model::Model(const std::filesystem::path& filename)
{
    // 存储顶点位置数据 (格式: v x y z)
    std::vector<glm::vec3> positions;
    
    // 存储顶点法线数据 (格式: vn x y z)
    std::vector<glm::vec3> normals;
    
    // 打开OBJ文件
    std::ifstream file{ filename };
    if (!file.good())
    {
        std::cout << "打开文件失败！" << std::endl;
        return;
    }
    
    std::string line;      // 存储每行文本
    char trash;            // 用于丢弃不需要的字符（如'v', 'f', '/'等）
    
    // 逐行读取文件，直到文件末尾
    while (std::getline(file, line))
    {
        // 将字符串行转换为流，便于解析
        std::istringstream iss(line);
        
        // 检查行首是否为 "v " (顶点位置)
        if (line.compare(0, 2, "v ") == 0)
        {
            glm::vec3 position;
            // 格式: v x y z
            // 丢弃第一个字符'v'，然后读取三个坐标
            iss >> trash >> position.x >> position.y >> position.z;
            // 将读取的位置添加到positions向量中
            positions.push_back(position);
        }
        // 检查行首是否为 "vn " (顶点法线)
        else if(line.compare(0, 3, "vn ") == 0)
        {
            glm::vec3 normal;
            // 格式: vn x y z
            // 丢弃'v'和'n'两个字符，然后读取三个法线分量
            iss >> trash >> trash >> normal.x >> normal.y >> normal.z;
            // 将读取的法线添加到normals向量中
            normals.push_back(normal);
        }
        // 检查行首是否为 "f " (面)
        else if (line.compare(0, 2, "f ") == 0)
        {
            glm::ivec3 idx_v;   // 存储三个顶点的索引
            glm::ivec3 idx_vn;  // 存储三个法线的索引
            
            // 格式: f v1//vn1 v2//vn2 v3//vn3
            // 示例: f 1//3 2//2 3//1
            
            iss >> trash;  // 丢弃第一个字符'f'
            
            // 解析第一个顶点: v1//vn1
            // 读取顶点索引，跳过'/'，跳过另一个'/'，读取法线索引
            iss >> idx_v.x >> trash >> trash >> idx_vn.x;
            
            // 解析第二个顶点: v2//vn2
            iss >> idx_v.y >> trash >> trash >> idx_vn.y;
            
            // 解析第三个顶点: v3//vn3
            iss >> idx_v.z >> trash >> trash >> idx_vn.z;
            
            // 检查索引是否有效（OBJ索引从1开始，需要转换为0基索引）
            if (idx_v.x <= positions.size() && idx_v.y <= positions.size() && idx_v.z <= positions.size() &&
                idx_vn.x <= normals.size() && idx_vn.y <= normals.size() && idx_vn.z <= normals.size())
            {
                // 创建三角形并添加到triangles向量
                // 注意：OBJ索引从1开始，所以需要减1转换为数组索引
                triangles.push_back(Triangle(
                    // 三个顶点的位置
                    positions[idx_v.x - 1], 
                    positions[idx_v.y - 1], 
                    positions[idx_v.z - 1],
                    // 三个顶点的法线
                    normals[idx_vn.x - 1], 
                    normals[idx_vn.y - 1], 
                    normals[idx_vn.z - 1]
                ));
            }
        }
        // 忽略其他行（如 vt 纹理坐标、注释行等）
    }
}

//遍历所有三角形，取最近的交点
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
