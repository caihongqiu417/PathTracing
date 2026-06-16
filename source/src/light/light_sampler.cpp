
#include "light_sampler.h"

// 根据 功率权重数组 生成 别名表
void LightSampler::Build(float scene_radius)
{
	std::vector<float> phis;
	phis.reserve(lights.size());
	for (const auto* light : lights)
	{
		phis.push_back(light->Phi(scene_radius));
	}

	alias_table.Build(phis);
}

std::optional<LightSourceSample> LightSampler::Sample(float u)const {
	if (lights.empty())
		return std::nullopt;

	auto sample_result = alias_table.Sample(u);
	return LightSourceSample{ lights[sample_result.index],sample_result.prob };
}
