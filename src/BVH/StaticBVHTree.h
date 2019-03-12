#pragma once

#include <iostream>
#include <vector>

#include "BVH/BVHNode.h"
#include "Primitives/SceneObjectList.h"
#include "Utils/ctpl.h"

class Microfacet;
class Surface;
class AABB;

class GpuTriangleList;
struct Ray;

namespace bvh
{
class StaticBVHTree : public WorldScene
{
  public:
    friend struct BVHNode;
    friend class MBVHNode;
    friend class MBVHTree;

    StaticBVHTree() = default;

    explicit StaticBVHTree(SceneObjectList *objectList, BVHType type = SAH,
                           ctpl::ThreadPool *pool = nullptr);
    explicit StaticBVHTree(GpuTriangleList *objectList, BVHType type = SAH,
                           ctpl::ThreadPool *pool = nullptr);

    void ConstructBVH() override;
    void BuildBVH();
    void Reset();
    void ResetGPU();
    BVHNode &GetNode(unsigned int idx);
    void TraceRay(Ray &r) const override;
    bool TraceShadowRay(Ray &r, float tMax) const override;
    void IntersectWithStack(Ray &r) const;
    const std::vector<SceneObject *> &GetLights() const override;
    AABB GetNodeBounds(unsigned int index) override;
    unsigned int GetPrimitiveCount() override;

    unsigned int TraceDebug(Ray &r) const override;

  public:
    std::vector<BVHNode> m_BVHPool;
    std::vector<unsigned int> m_PrimitiveIndices;
    size_t m_PrimitiveCount = 0;
    SceneObjectList *m_ObjectList = nullptr;
    GpuTriangleList *m_TriangleList = nullptr;
    bool CanUseBVH = false;
    unsigned int m_PoolPtr = 0;

  private:
    BVHType m_Type = SAH;
    ctpl::ThreadPool *m_ThreadPool = nullptr;
    std::mutex m_PoolPtrMutex{};
    std::mutex m_ThreadMutex{};
    unsigned int m_BuildingThreads = 0;
    bool m_ThreadLimitReached = false;

    std::vector<AABB> m_AABBs{};
};

struct BVHTraversal
{
    unsigned int nodeIdx;
    float tNear;
};
} // namespace bvh