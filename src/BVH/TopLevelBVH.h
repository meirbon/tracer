#pragma once

#include <future>
#include <vector>

#include "BVH/GameObject.h"
#include "BVH/MBVHTree.h"
#include "BVH/StaticBVHTree.h"
#include "Core/Renderer.h"
#include "Utils/ctpl.h"

namespace bvh
{
class MBVHTree;

class TopLevelBVH : public prims::WorldScene
{
  public:
	TopLevelBVH() = default;

	TopLevelBVH(prims::SceneObjectList *staticObjectList, std::vector<GameObject *> *gameObjectList,
				BVHType type = SAH_BINNING, ctpl::ThreadPool *tPool = nullptr);

	TopLevelBVH(prims::WorldScene *staticTree, prims::SceneObjectList *staticObjectList,
				std::vector<GameObject *> *gameObjectList, BVHType type = SAH_BINNING,
				ctpl::ThreadPool *tPool = nullptr);

	~TopLevelBVH() override;

	void ConstructDynamicBVH(int newDynamicTreeIndex);

	void TraceRay(core::Ray &r) const override;
	bool TraceShadowRay(core::Ray &r, float tMax) const override;

	void IntersectDynamic(core::Ray &r) const;

	void IntersectDynamicShadow(core::Ray &r) const;

	void IntersectDynamicWithStack(core::Ray &r) const;

	void UpdateDynamic(core::Renderer &renderer);
	void SwapDynamicTrees();

	const std::vector<prims::SceneObject *> &GetLights() const override;

	BVHType m_Type = SAH;
	ctpl::ThreadPool *m_ThreadPool = nullptr;
	WorldScene *m_StaticBVHTree = nullptr;
	prims::SceneObjectList *m_StaticObjectList = nullptr;

	std::vector<GameObject *> *gameObjectList = nullptr;
	std::vector<GameObjectNode> m_DynamicNodes;

	int GetActiveDynamicTreeIndex();
	int GetInActiveDynamicTreeIndex();
	std::future<void> ConstructNewDynamicBVHParallel(int newIndex);

	AABB GetNodeBounds(unsigned int index) override;
	uint GetPrimitiveCount() override;

	unsigned int TraceDebug(core::Ray &r) const override;

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