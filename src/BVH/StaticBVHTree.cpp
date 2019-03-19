#include "StaticBVHTree.h"
#include "Primitives/GpuTriangleList.h"
#include "Utils/Timer.h"

#define PRINT_BUILD_TIME 1
#define THREADING 1

namespace bvh
{
bvh::StaticBVHTree::StaticBVHTree(prims::SceneObjectList *objectList, BVHType type, ctpl::ThreadPool *pool)
{
	this->m_ObjectList = objectList;
	this->m_Type = type;
	this->m_ThreadPool = pool;
	Reset();
}

bvh::StaticBVHTree::StaticBVHTree(prims::GpuTriangleList *objectList, BVHType type, ctpl::ThreadPool *pool)
{
	this->m_TriangleList = objectList;
	this->m_Type = type;
	this->m_ThreadPool = pool;
	ResetGPU();
}

void bvh::StaticBVHTree::ConstructBVH()
{
	if (this->m_ObjectList != nullptr)
	{
		m_AABBs = m_ObjectList->GetAABBs();
		BuildBVH();
		return;
	}

	if (this->m_TriangleList != nullptr)
	{
		m_AABBs = m_TriangleList->GetAABBs();
		BuildBVH();
	}
}

void bvh::StaticBVHTree::BuildBVH()
{
	m_ThreadLimitReached = false;
	m_BuildingThreads = 0;

	if (m_PrimitiveCount > 0)
	{
#if PRINT_BUILD_TIME
		utils::Timer t;
		t.reset();
#endif
		this->m_PoolPtr = 2;
		auto &rootNode = m_BVHPool[0];
		rootNode.bounds.leftFirst = 0; // setting first
		rootNode.bounds.count = static_cast<int>(m_PrimitiveCount);
		rootNode.CalculateBounds(m_AABBs, m_PrimitiveIndices);

#if THREADING
		if (m_ThreadPool != nullptr)
		{
			rootNode.SubdivideMT(m_AABBs, this, 1);
		}
		else
		{
			rootNode.Subdivide(m_AABBs, this, 1);
		}
#else
		rootNode.Subdivide(m_AABBs, this, 1);
#endif

		if (m_PoolPtr > 2)
		{
			rootNode.bounds.count = -1;
			rootNode.SetLeftFirst(2);
		}
		else
		{
			rootNode.bounds.count = static_cast<int>(m_PrimitiveCount);
		}

#if PRINT_BUILD_TIME
		std::cout << "Building BVH took: " << t.elapsed() << " ms." << std::endl;
#endif
		CanUseBVH = true;
	}
}

void bvh::StaticBVHTree::Reset()
{
	CanUseBVH = false;
	m_BVHPool.clear();

	m_PrimitiveCount = m_ObjectList->GetPrimitiveCount();
	if (m_PrimitiveCount > 0)
	{
		m_PrimitiveIndices = m_ObjectList->GetPrimitiveIndices();
		m_BVHPool.resize(m_PrimitiveCount * 2);
	}
}

void bvh::StaticBVHTree::ResetGPU()
{
	CanUseBVH = false;
	m_BVHPool.clear();

	m_PrimitiveCount = m_TriangleList->GetPrimitiveCount();
	if (m_PrimitiveCount > 0)
	{
		m_PrimitiveIndices = m_TriangleList->GetPrimitiveIndices();
		m_BVHPool.resize(m_PrimitiveCount * 2);
	}
}

bvh::BVHNode &bvh::StaticBVHTree::GetNode(unsigned int idx) { return m_BVHPool[idx]; }

void bvh::StaticBVHTree::TraceRay(core::Ray &r) const
{
	if (CanUseBVH)
	{
		if (m_BVHPool[0].IntersectSIMD(r))
		{
			m_BVHPool[0].Traverse(r, m_ObjectList->GetObjects(), this);
		}
	}
	else
	{
		m_ObjectList->TraceRay(r);
	}

	r.normal = r.obj->GetNormal(r.GetHitpoint());
}

bool bvh::StaticBVHTree::TraceShadowRay(core::Ray &r, float tMax) const
{
	if (CanUseBVH)
	{
		if (m_BVHPool[0].IntersectSIMD(r))
		{
			m_BVHPool[0].Traverse(r, m_ObjectList->GetObjects(), this);
		}
	}
	else
	{
		m_ObjectList->TraceRay(r);
	}

	return r.t < tMax;
}

void bvh::StaticBVHTree::IntersectWithStack(core::Ray &r) const
{
	BVHTraversal todo[64];
	int stackptr = 0;
	float t1near, t1far;
	float t2near, t2far;
	const std::vector<prims::SceneObject *> &objects = m_ObjectList->GetObjects();

	// "Push" on the root node to the working set
	todo[stackptr].nodeIdx = 0;

	while (stackptr >= 0)
	{
		// Pop off the next node to work on.
		const unsigned int &nodeIdx = todo[stackptr].nodeIdx;
		const float &tNear = todo[stackptr].tNear;
		const BVHNode &node = m_BVHPool[nodeIdx];
		stackptr--;

		int leftFirst = node.bounds.leftFirst;
		int right = leftFirst + 1;

		// If this node is further than the closest found intersection, continue
		if (tNear > r.t)
			continue;

		if (node.IsLeaf())
		{ // Leaf node
			for (int idx = 0; idx < node.bounds.count; idx++)
			{
				objects[m_PrimitiveIndices[leftFirst + idx]]->Intersect(r);
			}
		}
		else
		{ // Not a leaf
			const bool hitLeftNode = m_BVHPool[leftFirst].IntersectSIMD(r, t1near, t1far);
			const bool hitRightNode = m_BVHPool[right].IntersectSIMD(r, t2near, t2far);

			if (hitLeftNode && hitRightNode)
			{
				if (t1near < t2far)
				{
					todo[++stackptr].nodeIdx = leftFirst;
					todo[stackptr].tNear = t1near;

					todo[++stackptr].nodeIdx = right;
					todo[stackptr].tNear = t2near;
				}
				else
				{
					todo[++stackptr].nodeIdx = right;
					todo[stackptr].tNear = t2near;

					todo[++stackptr].nodeIdx = leftFirst;
					todo[stackptr].tNear = t1near;
				}
			}
			else if (hitLeftNode)
			{
				todo[++stackptr].nodeIdx = leftFirst;
				todo[stackptr].tNear = t1near;
			}
			else if (hitRightNode)
			{
				todo[++stackptr].nodeIdx = right;
				todo[stackptr].tNear = t2near;
			}
		}
	}
}

const std::vector<prims::SceneObject *> &bvh::StaticBVHTree::GetLights() const { return m_ObjectList->GetLights(); }

unsigned int bvh::StaticBVHTree::GetPrimitiveCount() { return static_cast<unsigned int>(m_PrimitiveCount); }

unsigned int StaticBVHTree::TraceDebug(core::Ray &r) const
{
	if (CanUseBVH)
	{
		if (m_BVHPool[0].IntersectSIMD(r))
		{
			return m_BVHPool[0].TraverseDebug(r, m_BVHPool);
		}
	}

	return 0;
}

AABB bvh::StaticBVHTree::GetNodeBounds(unsigned int index) { return m_AABBs[index]; }
} // namespace bvh