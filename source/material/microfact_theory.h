#include<glm/glm.hpp>

//==============================================================
// 微表面模型:
//			Smith Model
//			GGX Distribution
//			伸缩不变性
//==============================================================
class MicrofactTheory
{
public:
	MicrofactTheory(float alpha_x, float alpha_z);

	/// @brief GGX 法线分布函数 D(m)
	///         描述微表面上法线朝向 microfacet_normal 的统计概率密度
	/// @param microfact_normal 微表面法线方向
	/// @return 概率密度值（单位 1/sr）
	float NormalDistribution(const glm::vec3& microfact_normal)const;

	/// @brief Smith 单向遮蔽函数 G1(wo, wh)
	///        从 view_direction 方向看，microfacet_normal 朝向的微表面
	///        有多大比例没有被其他微表面遮挡（可见比例）
	/// @param view_direction -ray.direction，入射方向的反方向
	/// @param mirofact_normal 微表面法线方向
	/// @return 可见比例 [0, 1]，0 = 完全遮蔽，1 = 完全可见
	float Masking(const glm::vec3& view_direction,
					const glm::vec3& mirofact_normal)const;

	/// @brief 高度相关 Smith 联合遮蔽-阴影函数 G2(wi, wo, wh)
	///        同时考虑两个方向的遮挡：
	///        遮蔽（Masking）：微表面被其他微表面遮住，view_direction 看不到
	///        阴影（Shadowing）：微表面被其他微表面遮住，light_direction 照不到
	/// @param light_direction 	wi: 光从光源来的方向 
	/// @param view_direction   wo: 光离开表面去相机的方向 
	/// @param microfact_normal 微表面法线方向
	/// @return 联合可见比例 [0, 1]，0 = 完全遮蔽或在阴影中，1 = 完全可见
	float HeightCorrelatedMaskingShadowing(const glm::vec3& light_direction, const glm::vec3& view_direction,
		const glm::vec3& microfact_normal);

		
	// 判断是否是Delta分布
	//  alpha_x == alpha_z ==0 => delta distribution
	bool IsDeltaDistribution()const;

private:
	// 拉伸前的形状函数，被 NormalDistribution() 调用
	// D(m) = P22(slope) / cos⁴(θh)，其中 slope = (-wh.x/wh.y, -wh.z/wh.y)
	// 各向异性 GGX：P22(sx,sz) = 1 / (π·αx·αz·(1 + sx²/αx² + sz²/αz²)²)
	float SlopeDistribution(const glm::vec2& slope) const;

	// Smith 辅助函数 Λ(w)，被 Masking() 和 HeightCorrelatedMaskingShadowing() 调用
	// G1(w) = 1 / (1 + Λ(w))
	// G2    = 1 / (1 + Λ(wi) + Λ(wo))
	// direction_upper：传入前已翻转到上半球（y > 0），避免 tan(θ) 出现负值
	float Lambda(const glm::vec3& direction_upper) const;



private:
	float alpha_x{};  // x 方向粗糙度（线性，= roughness²），控制高光在切线方向的拉伸
	float alpha_z{};  // z 方向粗糙度，alpha_x == alpha_z 时退化为各向同性
};