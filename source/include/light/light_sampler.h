#pragma once

#include"light.h"
#include"sample/alias_table.h"
#include<optional>
#include<map>


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

	//获取一个采样点对应的概率质量
	float GetProb(const Light* light)const { return Light2Prob.at(light); }
private:
	//管理光源数组
	std::vector<const Light*> lights;

	std::map<const Light*, float> Light2Prob;  //缓存灯光与对应的概率质量

	//别名表
	AliasTable alias_table;
};
