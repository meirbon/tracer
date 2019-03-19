#pragma once

#include "MBVHNode.h"
#include "Primitives/SceneObjectList.h"
#include "StaticBVHTree.h"
#include "Utils/ctpl.h"

namespace bvh
{
class AABB;
class MBVHNode;

struct MBVHTraversal
{
    int leftFirst; // Node
    int count;
    float tNear; // Minimum hit time for this node.
};

class MBVHTree : public prims::WorldScene
{
  public:
    friend class MBVHNode;

    MBVHTree() = default;

    MBVHTree(bvh::StaticBVHTree *orgTree);

    bvh::AABB m_Bounds = {glm::vec3(1e34f), glm::vec3(-1e34f)};
    prims::SceneObjectList *m_ObjectList = nullptr;
    bvh::StaticBVHTree *m_OriginalTree;
    std::vector<bvh::MBVHNode> m_Tree;
    std::vector<unsigned int> m_PrimitiveIndices{};
    bool m_CanUseBVH = false;
    unsigned int m_FinalPtr = 0;

    void Traverse(core::Ray &r) const;

    unsigned int TraverseDebug(core::Ray &r) const;

    void TraverseWithStack(core::Ray &r) const;

    void TraceRay(core::Ray &r) const override;

    bool TraceShadowRay(core::Ray &r, float tMax) const override;

    const std::vector<prims::SceneObject *> &GetLights() const override;

    void ConstructBVH() override;

    AABB GetNodeBounds(uint index) override;

    uint GetPrimitiveCount() override;

    unsigned int TraceDebug(core::Ray &r) const override;

  private:
    std::mutex m_PoolPtrMutex{};
    std::mutex m_ThreadMutex{};
    unsigned int m_BuildingThreads = 0;
    bool m_ThreadLimitReached = false;
};
} // namespace bvh