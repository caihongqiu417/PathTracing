#pragma once

#include"light.h"
#include"sample/alias_table.h"
#include<optional>


// 采样后返回的样本
struct LightSourceSample
{
	const Light* light;
	float prob;
};



// ================================================================
// 【 灯光采样器 】
//  lights → Phi[] → 别名表 → O(1) 按功率比例采样
// ================================================================
class LightSampler
{
public:
	void AddLight(const Light* light)
	{
		lights.push_back(light);
	}

	void Build(float scene_radius);

	std::optional<LightSourceSample> Sample(float u)const;


private:
	//管理光源数组
	std::vector<const Light*> lights;

	//别名表
	AliasTable alias_table;
};
