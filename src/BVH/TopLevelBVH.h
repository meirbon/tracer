#pragma once

#include <future>
#include <vector>

#include "BVH/GameObject.h"
#include "BVH/MBVHTree.h"
#include "BVH/StaticBVHTree.h"
#include "Utils/ctpl.h"

class SceneObjectList;
class Renderer;

namespace bvh
{
class MBVHTree;

class TopLevelBVH : public WorldScene
{
  public:
    TopLevelBVH() = default;

    TopLevelBVH(SceneObjectList *staticObjectList, std::vector<GameObject *> *gameObjectList,
                BVHType type = SAH_BINNING, ctpl::ThreadPool *tPool = nullptr);

    TopLevelBVH(WorldScene *staticTree, SceneObjectList *staticObjectList, std::vector<GameObject *> *gameObjectList,
                BVHType type = SAH_BINNING, ctpl::ThreadPool *tPool = nullptr);

    ~TopLevelBVH() override;

    void ConstructDynamicBVH(int newDynamicTreeIndex);

    void TraceRay(Ray &r) const override;
    bool TraceShadowRay(Ray &r, float tMax) const override;

    void IntersectDynamic(Ray &r) const;

    void IntersectDynamicShadow(Ray &r) const;

    void IntersectDynamicWithStack(Ray &r) const;

    void UpdateDynamic(Renderer &renderer);
    void SwapDynamicTrees();

    const std::vector<SceneObject *> &GetLights() const override;

    BVHType m_Type = SAH;
    ctpl::ThreadPool *m_ThreadPool = nullptr;
    WorldScene *m_StaticBVHTree = nullptr;
    SceneObjectList *m_StaticObjectList = nullptr;

    std::vector<GameObject *> *gameObjectList = nullptr;
    std::vector<GameObjectNode> m_DynamicNodes;

    int GetActiveDynamicTreeIndex();
    int GetInActiveDynamicTreeIndex();
    std::future<void> ConstructNewDynamicBVHParallel(int newIndex);

    AABB GetNodeBounds(unsigned int index) override;
    uint GetPrimitiveCount() override;

    unsigned int TraceDebug(Ray &r) const override;

  private:
    void FlattenGameObjects();

    void FlattenGameObjects(GameObject *currentObject, glm::mat4 currentTransform, glm::mat4 currentInverse,
                            std::vector<GameObjectNode> &dynamicNodes);

    void ConstructBVH() override;
    void UpdateDynamic(BVHNode *caller);

    std::mutex m_ThreadMutex{}, m_PartitionMutex{};

    int m_DynamicTreeIndex = 0;
    std::vector<bvh::AABB> m_DynamicAABBs{};
    std::vector<BVHNode> m_DynamicBVHTree[2] = {{}, {}};
    std::vector<unsigned int> m_DynamicIndices[2] = {{}, {}};
    bool CanUseDynamicBVH[2] = {false, false};
};
} // namespace bvh