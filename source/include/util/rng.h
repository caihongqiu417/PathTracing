#pragma once

#include <cstdint>
#include <random>

#include <glm/glm.hpp>

// 统一管理路径追踪里用到的随机数
class Rng
{
public:
	// 用固定种子初始化，保证同一个像素每次渲染结果稳定。
	explicit Rng(std::uint32_t seed)
		: engine(seed)
	{
	}

	// 返回 [0, 1] 内的随机数。
	float Float01() const
	{
		return uniform_01(engine);
	}

	// 返回 [-1, 1] 内的随机数，给方向采样用。
	float FloatSigned() const
	{
		return uniform_signed(engine);
	}

	// 给主光线一个像素内偏移，减少锯齿。
	glm::vec2 PixelOffset() const
	{
		return { Float01(), Float01() };
	}

	// 在局部上半球里随机取一个方向。
	glm::vec3 SampleHemisphere() const
	{
		glm::vec3 direction{};
		do
		{
			// 先在单位球里随机取点，不在球内就重试。
			direction = glm::vec3(FloatSigned(), FloatSigned(), FloatSigned());
		} while (glm::length(direction) > 1.f);

		// 折到 y >= 0 的半球，和局部法线方向保持一致。
		if (direction.y < 0.f)
			direction.y = -direction.y;

		return direction;
	}

private:
	// 随机数引擎本体。
	mutable std::mt19937 engine;
	// 生成 [0, 1] 的浮点数。
	mutable std::uniform_real_distribution<float> uniform_01{ 0.f, 1.f };
	// 生成 [-1, 1] 的浮点数。
	mutable std::uniform_real_distribution<float> uniform_signed{ -1.f, 1.f };
};
