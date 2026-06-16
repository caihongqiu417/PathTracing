#include "alias_table.h"

void AliasTable::Build(const std::vector<float>& values)
{
	size_t n = values.size();

	// 归一化权重，得到每个元素的概率
	float sum = 0;
	for (float v : values)
		sum += v;

	probs.resize(n);
	items.resize(n);

	std::vector<size_t> less, greater;

	// 初始化每一格：q=1 表示默认整格都是本格，p = probs[i]*n 表示相对于平均大小的比例
	for (size_t i = 0; i < n; i++)
	{
		probs[i]       = values[i] / sum;
		items[i].q     = 1.f;
		items[i].alias = -1;
		items[i].p     = probs[i] * static_cast<float>(n);

		if (items[i].p < 1.f)
			less.push_back(i);    // 概率不足一格，需要别人来补
		else if (items[i].p > 1.f)
			greater.push_back(i); // 概率超过一格，有多余可以分出去
	}

	// Walker's Alias Method 核心：将所有格子配对，使每格恰好装满 1.0
	// 不变量：每轮循环后，less/greater 队列中所有 p 之和保持不变
	while (!less.empty() && !greater.empty())
	{
		size_t l_idx = less.back();    less.pop_back();
		size_t g_idx = greater.back(); greater.pop_back();

		auto& item_l = items[l_idx];
		auto& item_g = items[g_idx];

		// 用 greater 格填满 less 格：
		//   [0, item_l.p) 采样到事件 l_idx 自身
		//   [item_l.p, 1) 采样到事件 g_idx（alias）
		item_l.q     = item_l.p;              
		item_l.alias = static_cast<int>(g_idx);

		// greater相应减少自身剩余
		item_g.p -= 1.f - item_l.q;

		if (item_g.p < 1.f)
			less.push_back(g_idx);    // 减少变成了 less，推回 less 队列等待被填
		else if (item_g.p > 1.f)
			greater.push_back(g_idx); // 还有多余，推回 greater 队列继续借给别人
		// p == 1 表示该格恰好装满，无需再处理
	}
}

AliasTable::SampleResult AliasTable::Sample(float u) const
{
	size_t n = items.size();

	// u*n 的整数部分定位格子，小数部分与阈值 q 比较
	float  scaled = u * static_cast<float>(n);
	size_t idx    = static_cast<size_t>(scaled);
	float  frac   = scaled - static_cast<float>(idx);

	// 决定取index还是alias
	int result = (frac < items[idx].q) ? static_cast<int>(idx) : items[idx].alias;

	return { result, u };
}
