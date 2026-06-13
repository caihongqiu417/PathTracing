#pragma once

#include "renderer/base_renderer.h"

// BoundsTest：包围盒检测次数热力图（SceneBVH + 模型BVH 合计）
class BoundsTestRenderer : public BaseRenderer {
public:
    BoundsTestRenderer(Camera& camera, const Scene& scene)
        : BaseRenderer(camera, scene) {}
private:
    const char* RendererName() const override { return "BVH Bounds Test Heatmap"; }
    glm::vec3 RenderPixel(const glm::ivec2& pixel_coord) override;

    bool  max_value_cached = false;
    float max_bounds       = 1.f;
};

// TriangleTest：三角形检测次数热力图
class TriangleTestRenderer : public BaseRenderer {
public:
    TriangleTestRenderer(Camera& camera, const Scene& scene)
        : BaseRenderer(camera, scene) {}
private:
    const char* RendererName() const override { return "Triangle Test Heatmap"; }
    glm::vec3 RenderPixel(const glm::ivec2& pixel_coord) override;

    bool  max_value_cached = false;
    float max_triangles    = 1.f;
};

// BoundsDepth：BVH树遍历深度热力图
class BoundsDepthRenderer : public BaseRenderer {
public:
    BoundsDepthRenderer(Camera& camera, const Scene& scene)
        : BaseRenderer(camera, scene) {}
private:
    const char* RendererName() const override { return "BVH Tree Depth Heatmap"; }
    glm::vec3 RenderPixel(const glm::ivec2& pixel_coord) override;

    bool  max_value_cached = false;
    float max_depth        = 1.f;
};
