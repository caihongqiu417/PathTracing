#pragma once

#include"light.h"
#include"shape/shape.h"

class AreaLight :public Light
{
public:
	/// @brief 构造函数
	/// @param shape 面光源形状
	/// @param Le 光源表面的辐射亮度Radiance
	/// @param double_side 是否朝两面都发出光
	AreaLight(const Shape& shape, const glm::vec3& Le, bool double_side)
		:shape(shape), Light(Le), double_side(double_side)
	{

	}

	// 获取光源功率
	virtual float Phi(float scene_radius)const override;

	// 从光源表面重要性采样一个方向，返回采样点
	virtual std::optional<LightSample> SampleLight(const glm::vec3& surface_point, float scene_radius,
													Rng& rng)const override;

	// 返回光源处辐射亮度
	glm::vec3 GetRadiance(const glm::vec3& surface_point, const glm::vec3& light_point, const glm::vec3& normal)const override;

	// 获取面积光的形状
	const Shape& GetShape() const { return shape; }

	virtual float GetPDF(const glm::vec3& surface_point, const glm::vec3& light_point, const glm::vec3& normal)const override;
private:
	const Shape& shape;
	bool double_side;
};