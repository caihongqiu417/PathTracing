#pragma once

#include<vector>

// Build O(n)，Sample O(1) 根据光源的功率大小，重要性采样光源
//
// 示例：weights=[1,2,3]，n=3，归一化 probs=[1/6, 2/6, 3/6]
// Build：缩放 p[i]=probs[i]*n -> [0.5, 1.0, 1.5]，item0(Less) 用 item2(Greater) 填满
//
//   +---+-----+-------+------------------------------------------+
//   | i |  q  | alias | Sample 行为（u 落在第 i 格时）           |
//   +---+-----+-------+------------------------------------------+
//   | 0 | 0.5 |   2   | frac < 0.5 -> 事件0，否则 -> 事件2      |
//   | 1 | 1.0 |  -1   | 始终 -> 事件1                           |
//   | 2 | 1.0 |  -1   | 始终 -> 事件2                           |
//   +---+-----+-------+------------------------------------------+
class AliasTable
{
private:
	struct Item
	{
		float q;    // 小数部分 < q 时取本格事件，否则取 alias 格事件
		float p;    // Build 专用：缩放概率 probs[i]*n，均值为 1；< 1 为 Less，> 1 为 Greater
		int alias;  // 备用事件下标
	};

public:
	struct SampleResult
	{
		int   index; // 采样到的事件下标
		float u;     // 原始随机数，供外部复用（如 MIS 权重计算）
	};

	AliasTable() = default;

	// 按 values 的权重比例构建别名表
	void Build(const std::vector<float>& values);

	// u 为 [0,1) 均匀随机数，返回按权重采样的事件下标及原始 u
	SampleResult Sample(float u) const;

private:
	std::vector<float> probs; // 归一化概率，probs[i] = values[i] / sum
	std::vector<Item>  items; // 别名表
};
