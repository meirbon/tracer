#pragma once

#include <glm/glm.hpp>

#include "BVH/StaticBVHTree.h"
#include "Materials/Material.h"
#include "Primitives/SceneObjectList.h"
#include "Renderer/Surface.h"

using namespace glm;

struct GpuTriangle
{
    vec3 p0;      // 12
    float dummy0; // 16

    vec3 p1;      // 28
    float dummy1; // 32

    vec3 p2;
    float dummy2; // 48

    vec3 n0;
    float dummy3; // 64

    vec3 n1;
    float dummy4; // 80

    vec3 n2;
    float dummy5; // 96

    vec3 normal;
    float dum; // 112

    uint matIdx;  // 116
    uint objIdx;  // 120
    float m_Area; // 124
    float dummy;  // 128

    vec2 t0, t1, t2;  // 152
    float d156, d160; // 160

    GpuTriangle() = default;

    GpuTriangle(vec3 p0, vec3 p1, vec3 p2, uint matIndex);

    GpuTriangle(vec3 p0, vec3 p1, vec3 p2, uint matIndex, vec2 t0, vec2 t1,
                vec2 t2);

    GpuTriangle(vec3 p0, vec3 p1, vec3 p2, vec3 n0, vec3 n1, vec3 n2,
                uint matIndex);

    GpuTriangle(vec3 p0, vec3 p1, vec3 p2, vec3 n0, vec3 n1, vec3 n2,
                uint matIndex, vec2 t0, vec2 t1, vec2 t2);

    vec3 GetCentroid() const;

    bvh::AABB GetBounds() const;

    float CalcArea() const;
};

class GpuTriangleList : public WorldScene
{
  public:
    GpuTriangleList() = default;

    ~GpuTriangleList() override = default;

    void AddTriangle(GpuTriangle triangle);

    void AddLight(GpuTriangle triangle);

    inline const std::vector<GpuTriangle> &GetTriangles() const
    {
        return m_Triangles;
    }

    inline const std::vector<bvh::AABB> &GetAABBs() const { return m_Aabbs; }
    inline std::vector<uint> &GetLightIndices() { return m_LightIndices; }

    inline GpuTriangle &GetTriangle(uint idx) { return m_Triangles[idx]; }

    std::vector<unsigned int> GetPrimitiveIndices() { return m_PrimIndices; }

    unsigned int GetPrimitiveCount() override;

    inline bvh::AABB GetNodeBounds(unsigned int index) override
    {
        return m_Triangles[index].GetBounds();
    }

    void TraceRay(Ray &r) const override;

    bool TraceShadowRay(Ray &r, float tMax) const override;

    unsigned int TraceDebug(Ray &r) const override;

  private:
    std::vector<GpuTriangle> m_Triangles{};
    std::vector<bvh::AABB> m_Aabbs{};
    std::vector<uint> m_LightIndices{};
    std::vector<unsigned int> m_PrimIndices{};

    const std::vector<SceneObject *> &GetLights() const override;

    void ConstructBVH() override;
};
