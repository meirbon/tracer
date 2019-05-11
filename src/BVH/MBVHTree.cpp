#include "MBVHTree.h"
#include "MBVHNode.h"

#include <glm/ext.hpp>

#define PRINT_BUILD_TIME 1
#define THREADING 0

#if PRINT_BUILD_TIME
#include <Utils/Timer.h>
#endif

namespace bvh
{
MBVHTree::MBVHTree(StaticBVHTree *orgTree)
{
	this->m_ObjectList = orgTree->m_ObjectList;
	this->m_PrimitiveIndices = orgTree->m_PrimitiveIndices;
	this->m_OriginalTree = orgTree;
	ConstructBVH();
}

void MBVHTree::Traverse(core::Ray &r) const
{
	if (m_CanUseBVH)
	{
		const vec3 invDir = 1.f / r.direction;
		m_Tree[0].Intersect(r, invDir, m_Tree.data(), m_PrimitiveIndices, m_ObjectList->GetObjects());
	}
}

unsigned int MBVHTree::TraverseDebug(core::Ray &r) const
{
	if (m_CanUseBVH)
	{
		const vec3 invDir = 1.f / r.direction;
		return m_Tree[0].IntersectDebug(r, invDir, m_Tree.data());
	}

	return 0;
}

void MBVHTree::ConstructBVH()
{
	const uint primCount = m_OriginalTree->m_AABBs.size();
	if (primCount <= 0)
		return;

	m_CanUseBVH = false;
	m_Tree.clear();
	m_Tree.resize(primCount * 2);
#if PRINT_BUILD_TIME
	utils::Timer t{};
#endif
	m_FinalPtr = 1;
	MBVHNode &mRootNode = m_Tree[0];
	BVHNode &curNode = m_OriginalTree->GetNode(0);
#if THREADING // threading is not actually faster here
	// Only use threading when we have a large number of primitives,
	// otherwise threading is actually slower
	if (m_OriginalTree->m_ThreadPool != nullptr && m_OriginalTree->GetPrimitiveCount() >= 250000)
	{
		m_ThreadLimitReached = false;
		m_BuildingThreads = 0;
		mRootNode.MergeNodesMT(curNode, this->m_OriginalTree->m_BVHPool, this);
	}
	else
	{
		mRootNode.MergeNodes(curNode, this->m_OriginalTree->m_BVHPool, this);
	}
#else
	mRootNode.MergeNodes(curNode, this->m_OriginalTree->m_BVHPool, this);
#endif
	m_Bounds = m_OriginalTree->GetNode(0).bounds;

#if PRINT_BUILD_TIME
	std::cout << "Building MBVH took: " << t.elapsed() << " ms." << std::endl;
#endif
	m_Tree.resize(m_FinalPtr);
	m_CanUseBVH = true;
}

void MBVHTree::TraceRay(core::Ray &r) const
{
	Traverse(r);
	//	TraverseWithStack(r);
	if (r.IsValid())
		r.normal = r.obj->GetNormal(r.GetHitpoint());
}

bool MBVHTree::TraceShadowRay(core::Ray &r, float tMax) const
{
	Traverse(r);
	//	TraverseWithStack(r);
	return r.t < tMax;
}

const std::vector<prims::SceneObject *> &MBVHTree::GetLights() const { return m_ObjectList->GetLights(); }

AABB MBVHTree::GetNodeBounds(uint index)
{
	MBVHNode &curNode = m_Tree[index];
	const float maxx =
		glm::max(curNode.bmaxx[0], glm::max(curNode.bmaxx[1], glm::max(curNode.bmaxx[2], curNode.bmaxx[3])));
	const float maxy =
		glm::max(curNode.bmaxy[0], glm::max(curNode.bmaxy[1], glm::max(curNode.bmaxy[2], curNode.bmaxy[3])));
	const float maxz =
		glm::max(curNode.bmaxz[0], glm::max(curNode.bmaxz[1], glm::max(curNode.bmaxz[2], curNode.bmaxz[3])));

	const float minx =
		glm::min(curNode.bminx[0], glm::min(curNode.bminx[1], glm::min(curNode.bminx[2], curNode.bminx[3])));
	const float miny =
		glm::min(curNode.bminy[0], glm::min(curNode.bminy[1], glm::min(curNode.bminy[2], curNode.bminy[3])));
	const float minz =
		glm::min(curNode.bminz[0], glm::min(curNode.bminz[1], glm::min(curNode.bminz[2], curNode.bminz[3])));

	return AABB(vec3(minx, miny, minz), vec3(maxx, maxy, maxz));
}

uint MBVHTree::GetPrimitiveCount() { return this->m_OriginalTree->GetPrimitiveCount(); }

unsigned int MBVHTree::TraceDebug(core::Ray &r) const { return TraverseDebug(r); }

void MBVHTree::TraverseWithStack(core::Ray &r) const
{
	MBVHTraversal todo[32];
	int stack_ptr = 0;

	todo[0].leftFirst = 0;
	todo[0].count = -1;

	const vec3 invDir = 1.f / r.direction;
	const std::vector<prims::SceneObject *> &objects = m_ObjectList->GetObjects();

	while (stack_ptr >= 0)
	{
		const MBVHTraversal &mTodo = todo[stack_ptr];
		stack_ptr--;
		if (mTodo.count > -1) // leaf node
			for (int i = 0; i < mTodo.count; i++)
				objects[m_PrimitiveIndices[mTodo.leftFirst + i]]->Intersect(r);
		else
		{
			const auto &n = this->m_Tree[mTodo.leftFirst];
			auto mHit = m_Tree[mTodo.leftFirst].Intersect(r, invDir);
			if (glm::any(mHit.result))
			{
				for (int i = 3; i >= 0; i--)
				{
					// reversed order, we want to check best nodes first
					const unsigned int idx = (mHit.tmini[i] & 0b11);
					if (mHit.result[idx])
					{
						stack_ptr++;
						todo[stack_ptr].leftFirst = n.child[idx];
						todo[stack_ptr].count = n.count[idx];
					}
				}
			}
		}
	}
}
} // namespace bvh