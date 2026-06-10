#pragma once

#include <filesystem>

#include "camera/camera.h"
#include "shape/shape.h"
#include "shape/scene.h"
#include "util/rng.h"

// 快速生成一个渲染器类。
#define DEFINE_RENDERER(Name, DisplayName)\
	class Name##Renderer:public BaseRenderer{\
	public:\
		Name##Renderer(Camera& camera, const Scene& scene)\
			: BaseRenderer(camera, scene)\
		{\
		}\
	private:\
		const char* RendererName() const override { return DisplayName; }\
		glm::vec3 RenderPixel(const glm::ivec2& pixel_coord) override;\
	};\



// 渲染器基类。
class BaseRenderer
{
public:
	BaseRenderer(Camera& camera, const Scene& scene)
		: camera(camera), scene(scene), rng(0u)
	{
	}

	virtual ~BaseRenderer() = default;

	// 渲染整张图。
	void Render(size_t spp, const std::filesystem::path& filename);

private:
	// 进度条里显示的名字。
	virtual const char* RendererName() const = 0;
	// 算一个样本的颜色。
	virtual glm::vec3 RenderPixel(const glm::ivec2& pixel_coord) = 0;

protected:
	Camera& camera;
	const Scene& scene;
	// 当前样本用的随机数。
	Rng rng;
};