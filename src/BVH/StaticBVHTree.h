#pragma once

#include <iostream>
#include <vector>

#include "BVH/BVHNode.h"
#include "Primitives/SceneObjectList.h"
#include "Utils/ctpl.h"

class Microfacet;
namespace core
{
class Surface;
struct Ray;
} // namespace core

namespace bvh
{
class AABB;
class StaticBVHTree : public prims::WorldScene
{
  public:
	friend struct BVHNode;
	friend class MBVHNode;
	friend class MBVHTree;

	explicit StaticBVHTree(std::vector<AABB> aabbs, BVHType type = SAH, ctpl::ThreadPool *pool = nullptr);
	explicit StaticBVHTree(prims::SceneObjectList *objectList, BVHType type = SAH, ctpl::ThreadPool *pool = nullptr);
	StaticBVHTree() = default;

	void ConstructBVH() override;
	void BuildBVH();
	void Reset();
	BVHNode &GetNode(unsigned int idx);
	void TraceRay(core::Ray &r) const override;
	bool TraceShadowRay(core::Ray &r, float tMax) const override;
	void IntersectWithStack(core::Ray &r) const;
	const std::vector<prims::SceneObject *> &GetLights() const override;
	AABB GetNodeBounds(unsigned int index) override;
	unsigned int GetPrimitiveCount() override;

	unsigned int TraceDebug(core::Ray &r) const override;

  public:
	std::vector<BVHNode> m_BVHPool;
	std::vector<unsigned int> m_PrimitiveIndices;
	size_t m_PrimitiveCount = 0;
	prims::SceneObjectList *m_ObjectList = nullptr;
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