#include "BVHNode.h"
#include "StaticBVHTree.h"

#include <glm/ext.hpp>

#define MAX_PRIMS 4
#define MAX_DEPTH 64
#define BINS 11

const __m128 QuadOne = _mm_set1_ps(1.f);

bvh::BVHNode::BVHNode()
{
    SetLeftFirst(-1);
    SetCount(-1);
}

bvh::BVHNode::BVHNode(int leftFirst, int count, AABB bounds)
    : bounds(bounds)
{
    SetLeftFirst(leftFirst);
    SetCount(-1);
}

bool bvh::BVHNode::Intersect(const core::Ray& r) const
{
    const float tx1 = (bounds.bmin[0] - r.origin.x) / r.direction.x;
    const float tx2 = (bounds.bmax[0] - r.origin.x) / r.direction.x;

    float tmin = glm::min(tx1, tx2);
    float tmax = glm::max(tx1, tx2);

    const float ty1 = (bounds.bmin[1] - r.origin.y) / r.direction.y;
    const float ty2 = (bounds.bmax[1] - r.origin.y) / r.direction.y;

    tmin = glm::max(tmin, glm::min(ty1, ty2));
    tmax = glm::min(tmax, glm::max(ty1, ty2));

    const float tz1 = (bounds.bmin[2] - r.origin.z) / r.direction.z;
    const float tz2 = (bounds.bmax[2] - r.origin.z) / r.direction.z;

    tmin = glm::max(tmin, glm::min(tz1, tz2));
    tmax = glm::min(tmax, glm::max(tz1, tz2));

    return tmax >= 0 && tmin < tmax;
}

bool bvh::BVHNode::IntersectSIMD(const core::Ray& r) const
{
    const __m128 dirInversed = _mm_div_ps(QuadOne, _mm_load_ps(glm::value_ptr(r.direction)));
    const __m128 t1 = _mm_mul_ps(_mm_sub_ps(bounds.bmin4, _mm_load_ps(glm::value_ptr(r.origin))), dirInversed);
    const __m128 t2 = _mm_mul_ps(_mm_sub_ps(bounds.bmax4, _mm_load_ps(glm::value_ptr(r.origin))), dirInversed);

    union {
        __m128 f4;
        float f[4];
    } qvmax, qvmin;

    qvmax.f4 = _mm_max_ps(t1, t2);
    qvmin.f4 = _mm_min_ps(t1, t2);

    const float tmax = glm::min(qvmax.f[0], glm::min(qvmax.f[1], qvmax.f[2]));
    const float tmin = glm::max(qvmin.f[0], glm::max(qvmin.f[1], qvmin.f[2]));

    return tmax >= 0 && tmin < tmax;
}

bool bvh::BVHNode::Intersect(const core::Ray& r, float& tNear, float& tFar) const
{
    const float tx1 = (bounds.bmin[0] - r.origin.x) / r.direction.x;
    const float tx2 = (bounds.bmax[0] - r.origin.x) / r.direction.x;

    tNear = glm::min(tx1, tx2);
    tFar = glm::max(tx1, tx2);

    const float ty1 = (bounds.bmin[1] - r.origin.y) / r.direction.y;
    const float ty2 = (bounds.bmax[1] - r.origin.y) / r.direction.y;

    tNear = glm::max(tNear, glm::min(ty1, ty2));
    tFar = glm::min(tFar, glm::max(ty1, ty2));

    const float tz1 = (bounds.bmin[2] - r.origin.z) / r.direction.z;
    const float tz2 = (bounds.bmax[2] - r.origin.z) / r.direction.z;

    tNear = glm::max(tNear, glm::min(tz1, tz2));
    tFar = glm::min(tFar, glm::max(tz1, tz2));

    return tFar >= 0 && tNear < tFar;
}

bool bvh::BVHNode::IntersectSIMD(const core::Ray& r, float& tNear, float& tFar) const
{
    const __m128 dirInversed = _mm_div_ps(QuadOne, _mm_load_ps(glm::value_ptr(r.direction)));
    const __m128 t1 = _mm_mul_ps(_mm_sub_ps(bounds.bmin4, _mm_load_ps(glm::value_ptr(r.origin))), dirInversed);
    const __m128 t2 = _mm_mul_ps(_mm_sub_ps(bounds.bmax4, _mm_load_ps(glm::value_ptr(r.origin))), dirInversed);

    union {
        __m128 f4;
        float f[4];
    } qvmax, qvmin;

    qvmax.f4 = _mm_max_ps(t1, t2);
    qvmin.f4 = _mm_min_ps(t1, t2);

    tFar = glm::min(qvmax.f[0], glm::min(qvmax.f[1], qvmax.f[2]));
    tNear = glm::max(qvmin.f[0], glm::max(qvmin.f[1], qvmin.f[2]));

    return tFar >= 0 && tNear < tFar;
}

unsigned int bvh::BVHNode::TraverseDebug(core::Ray& r, const bvh::StaticBVHTree* bvhTree) const
{
    unsigned int depth = 1;

    if (!this->IsLeaf()) {
        float tNearLeft, tFarLeft;
        float tNearRight, tFarRight;
        const bool hitLeft = bvhTree->m_BVHPool[bounds.leftFirst].IntersectSIMD(r, tNearLeft, tFarLeft);
        const bool hitRight = bvhTree->m_BVHPool[bounds.leftFirst + 1].IntersectSIMD(r, tNearRight, tFarRight);

        if (hitLeft && hitRight) {
            if (tNearLeft < tNearRight) {
                depth += bvhTree->m_BVHPool[bounds.leftFirst].TraverseDebug(r, bvhTree);
                if (r.t < tNearRight && r.IsValid())
                    return depth;
                depth += bvhTree->m_BVHPool[bounds.leftFirst + 1].TraverseDebug(r, bvhTree);
            } else {
                depth += bvhTree->m_BVHPool[bounds.leftFirst + 1].TraverseDebug(r, bvhTree);
                if (r.t < tNearLeft && r.IsValid())
                    return depth;
                depth += bvhTree->m_BVHPool[bounds.leftFirst].TraverseDebug(r, bvhTree);
            }
        } else {
            if (hitLeft)
                depth += bvhTree->m_BVHPool[bounds.leftFirst].TraverseDebug(r, bvhTree);
            else if (hitRight)
                depth += bvhTree->m_BVHPool[bounds.leftFirst + 1].TraverseDebug(r, bvhTree);
        }
    }

    return depth;
}

unsigned int bvh::BVHNode::TraverseDebug(core::Ray& r, const std::vector<BVHNode>& bvhTree) const
{
    unsigned int depth = 1;
    if (!this->IsLeaf()) {
        float tNearLeft, tFarLeft;
        float tNearRight, tFarRight;
        const bool hitLeft = bvhTree[bounds.leftFirst].IntersectSIMD(r, tNearLeft, tFarLeft);
        const bool hitRight = bvhTree[bounds.leftFirst + 1].IntersectSIMD(
            r, tNearRight, tFarRight);

        if (hitLeft && hitRight) {
            if (tNearLeft < tNearRight) {
                depth += bvhTree[bounds.leftFirst].TraverseDebug(r, bvhTree);
                if (r.t < tNearRight && r.IsValid())
                    return depth;
                depth += bvhTree[bounds.leftFirst + 1].TraverseDebug(r, bvhTree);
            } else {
                depth += bvhTree[bounds.leftFirst + 1].TraverseDebug(r, bvhTree);
                if (r.t < tNearLeft && r.IsValid())
                    return depth;
                depth += bvhTree[bounds.leftFirst].TraverseDebug(r, bvhTree);
            }
        } else {
            if (hitLeft)
                depth += bvhTree[bounds.leftFirst].TraverseDebug(r, bvhTree);
            else if (hitRight)
                depth += bvhTree[bounds.leftFirst + 1].TraverseDebug(r, bvhTree);
        }
    }

    return depth;
}

unsigned int bvh::BVHNode::TraverseDebug(core::Ray& r, const std::vector<bvh::GameObjectNode>& objectList, const std::vector<bvh::BVHNode>& bvhTree, const std::vector<unsigned int>& primIndices) const
{
    unsigned int depth = 1;
    if (this->IsLeaf()) {
        for (int idx = 0; idx < bounds.count; idx++) {
            const GameObjectNode& node = objectList[primIndices[idx]];
            if (node.Intersect(r)) {
                depth += node.TraverseDebug(r);
            }
        }
        return depth;
    }

    float tNearLeft, tFarLeft;
    float tNearRight, tFarRight;
    const bool hitLeft = bvhTree[bounds.leftFirst].Intersect(r, tNearLeft, tFarLeft);
    const bool hitRight = bvhTree[bounds.leftFirst + 1].Intersect(r, tNearRight, tFarRight);

    if (hitLeft && hitRight) {
        if (tNearLeft < tNearRight) {
            depth += bvhTree[bounds.leftFirst].TraverseDebug(r, objectList, bvhTree, primIndices);
            if (r.t < tNearRight && r.IsValid())
                return depth;
            depth += bvhTree[bounds.leftFirst + 1].TraverseDebug(r, objectList, bvhTree, primIndices);
        } else {
            depth += bvhTree[bounds.leftFirst + 1].TraverseDebug(r, objectList, bvhTree, primIndices);
            if (r.t < tNearLeft && r.IsValid())
                return depth;
            depth += bvhTree[bounds.leftFirst].Traverse(r, objectList, bvhTree, primIndices);
        }
    } else {
        if (hitLeft)
            depth += bvhTree[bounds.leftFirst].Traverse(r, objectList, bvhTree, primIndices);
        else if (hitRight)
            depth += bvhTree[bounds.leftFirst + 1].Traverse(r, objectList, bvhTree, primIndices);
    }

    return depth;
}

void bvh::BVHNode::Traverse(
    core::Ray& r,
    const std::vector<prims::SceneObject*>& objectList,
    const bvh::StaticBVHTree* bvhTree) const
{
    if (this->IsLeaf()) {
        for (int idx = 0; idx < bounds.count; idx++) {
            objectList[bvhTree->m_PrimitiveIndices[bounds.leftFirst + idx]]
                ->Intersect(r);
        }
    } else {
        float tNearLeft, tFarLeft;
        float tNearRight, tFarRight;
        const bool hitLeft = bvhTree->m_BVHPool[bounds.leftFirst].IntersectSIMD(r, tNearLeft, tFarLeft);
        const bool hitRight = bvhTree->m_BVHPool[bounds.leftFirst + 1].IntersectSIMD(
            r, tNearRight, tFarRight);

        if (hitLeft && hitRight) {
            if (tNearLeft < tNearRight) {
                bvhTree->m_BVHPool[bounds.leftFirst].Traverse(r, objectList, bvhTree);
                if (r.t < tNearRight && r.IsValid())
                    return;
                bvhTree->m_BVHPool[bounds.leftFirst + 1].Traverse(r, objectList, bvhTree);
            } else {
                bvhTree->m_BVHPool[bounds.leftFirst + 1].Traverse(r, objectList, bvhTree);
                if (r.t < tNearLeft && r.IsValid())
                    return;
                bvhTree->m_BVHPool[bounds.leftFirst].Traverse(r, objectList, bvhTree);
            }
        } else {
            if (hitLeft)
                bvhTree->m_BVHPool[bounds.leftFirst].Traverse(r, objectList, bvhTree);
            else if (hitRight)
                bvhTree->m_BVHPool[bounds.leftFirst + 1].Traverse(r, objectList, bvhTree);
        }
    }
}

void bvh::BVHNode::Traverse(
    core::Ray& r,
    const std::vector<prims::SceneObject*>& objectList,
    const std::vector<bvh::BVHNode>& bvhTree,
    const std::vector<unsigned int>& primIndices) const
{
    if (this->IsLeaf()) {
        for (int idx = 0; idx < bounds.count; idx++) {
            objectList[primIndices[bounds.leftFirst + idx]]
                ->Intersect(r);
        }
    } else {
        float tNearLeft, tFarLeft;
        float tNearRight, tFarRight;
        const bool hitLeft = bvhTree[bounds.leftFirst].IntersectSIMD(r, tNearLeft, tFarLeft);
        const bool hitRight = bvhTree[bounds.leftFirst + 1].IntersectSIMD(
            r, tNearRight, tFarRight);

        if (hitLeft && hitRight) {
            if (tNearLeft < tNearRight) {
                bvhTree[bounds.leftFirst].Traverse(r, objectList, bvhTree, primIndices);
                if (r.t < tNearRight && r.IsValid())
                    return;
                bvhTree[bounds.leftFirst + 1].Traverse(r, objectList, bvhTree, primIndices);
            } else {
                bvhTree[bounds.leftFirst + 1].Traverse(r, objectList, bvhTree, primIndices);
                if (r.t < tNearLeft && r.IsValid())
                    return;
                bvhTree[bounds.leftFirst].Traverse(r, objectList, bvhTree, primIndices);
            }
        } else {
            if (hitLeft)
                bvhTree[bounds.leftFirst].Traverse(r, objectList, bvhTree, primIndices);
            else if (hitRight)
                bvhTree[bounds.leftFirst + 1].Traverse(r, objectList, bvhTree, primIndices);
        }
    }
}

int bvh::BVHNode::Traverse(
    core::Ray& r,
    const std::vector<bvh::GameObjectNode>& objectList,
    const std::vector<bvh::BVHNode>& bvhTree,
    const std::vector<unsigned int>& primIndices) const
{
    int i = -1;
    if (this->IsLeaf()) {
        for (int idx = 0; idx < bounds.count; idx++) {
            const GameObjectNode& node = objectList[primIndices[idx]];
            if (node.Intersect(r)) {
                node.Traverse(r);
                i = bounds.leftFirst + idx;
            }
        }
        return i;
    }

    float tNearLeft, tFarLeft;
    float tNearRight, tFarRight;
    const bool hitLeft = bvhTree[bounds.leftFirst].Intersect(r, tNearLeft, tFarLeft);
    const bool hitRight = bvhTree[bounds.leftFirst + 1].Intersect(r, tNearRight, tFarRight);

    if (hitLeft && hitRight) {
        if (tNearLeft < tNearRight) {
            i = bvhTree[bounds.leftFirst].Traverse(r, objectList, bvhTree, primIndices);
            if (r.t < tNearRight && r.IsValid())
                return i;
            i = bvhTree[bounds.leftFirst + 1].Traverse(r, objectList, bvhTree, primIndices);
        } else {
            i = bvhTree[bounds.leftFirst + 1].Traverse(r, objectList, bvhTree, primIndices);
            if (r.t < tNearLeft && r.IsValid())
                return i;
            i = bvhTree[bounds.leftFirst].Traverse(r, objectList, bvhTree, primIndices);
        }
    } else {
        if (hitLeft)
            i = bvhTree[bounds.leftFirst].Traverse(r, objectList, bvhTree, primIndices);
        else if (hitRight)
            i = bvhTree[bounds.leftFirst + 1].Traverse(r, objectList, bvhTree, primIndices);
    }

    return i;
}

int bvh::BVHNode::TraverseShadow(
    core::Ray& r,
    const std::vector<bvh::GameObjectNode>& objectList,
    const std::vector<bvh::BVHNode>& bvhTree,
    const std::vector<unsigned int>& primIndices) const
{
    int i = -1;
    if (this->IsLeaf()) {
        for (int idx = 0; idx < bounds.count; idx++) {
            const GameObjectNode& node = objectList[primIndices[idx]];
            if (node.Intersect(r)) {
                node.TraverseShadow(r);
                i = bounds.leftFirst + idx;
            }
        }
        return i;
    }

    float tNearLeft, tFarLeft;
    float tNearRight, tFarRight;
    const bool hitLeft = bvhTree[bounds.leftFirst].Intersect(r, tNearLeft, tFarLeft);
    const bool hitRight = bvhTree[bounds.leftFirst + 1].Intersect(r, tNearRight, tFarRight);

    if (hitLeft && hitRight) {
        if (tNearLeft < tNearRight) {
            i = bvhTree[bounds.leftFirst].Traverse(r, objectList, bvhTree, primIndices);
            if (r.t < tNearRight && r.IsValid())
                return i;
            i = bvhTree[bounds.leftFirst + 1].Traverse(r, objectList, bvhTree, primIndices);
        } else {
            i = bvhTree[bounds.leftFirst + 1].Traverse(r, objectList, bvhTree, primIndices);
            if (r.t < tNearLeft && r.IsValid())
                return i;
            i = bvhTree[bounds.leftFirst].Traverse(r, objectList, bvhTree, primIndices);
        }
    } else {
        if (hitLeft)
            i = bvhTree[bounds.leftFirst].Traverse(r, objectList, bvhTree, primIndices);
        else if (hitRight)
            i = bvhTree[bounds.leftFirst + 1].Traverse(r, objectList, bvhTree, primIndices);
    }

    return i;
}

void bvh::BVHNode::Subdivide(const std::vector<AABB>& aabbs, StaticBVHTree* bvhTree,
    unsigned int depth)
{
    depth++;
    if (GetCount() < MAX_PRIMS || depth >= MAX_DEPTH)
        return; // this is a leaf node

    int left;
    int right;

    if (!Partition(aabbs, bvhTree, left, right)) {
        return;
    }

    this->bounds.leftFirst = left; // Set pointer to children
    this->bounds.count = -1; // No primitives since we are no leaf node

    auto& leftNode = bvhTree->m_BVHPool[left];
    auto& rightNode = bvhTree->m_BVHPool[right];

    if (leftNode.bounds.count > 0) {
        leftNode.CalculateBounds(aabbs, bvhTree->m_PrimitiveIndices);
        leftNode.Subdivide(aabbs, bvhTree, depth);
    }

    if (rightNode.bounds.count > 0) {
        rightNode.CalculateBounds(aabbs, bvhTree->m_PrimitiveIndices);
        rightNode.Subdivide(aabbs, bvhTree, depth);
    }
}

bool bvh::BVHNode::Partition(const std::vector<AABB>& aabbs, StaticBVHTree* bvhTree,
    int& left, int& right)
{
    const int lFirst = bounds.leftFirst;
    int lCount = 0;
    int rFirst = bounds.leftFirst;
    int rCount = bounds.count;

    float parentNodeCost{}, lowestNodeCost = 1e34f, bestCoord{};
    int bestAxis{};

    switch (bvhTree->m_Type) {
    case (SAH): {
        parentNodeCost = bounds.Area() * bounds.count;
        for (int idx = 0; idx < bounds.count; idx++) {
            for (int axis = 0; axis < 3; axis++) {
                const float splitCoord = aabbs.at(bvhTree->m_PrimitiveIndices[lFirst + idx])
                                             .Centroid()[axis];
                int leftCount = 0, rightCount = 0;
                AABB leftBox = AABB(vec3(1e34f), vec3(-1e34f));
                AABB rightBox = AABB(vec3(1e34f), vec3(-1e34f));

                for (int i = 0; i < bounds.count; i++) {
                    const auto& aabb = aabbs.at(bvhTree->m_PrimitiveIndices[lFirst + i]);
                    if (aabb.Centroid()[axis] <= splitCoord) {
                        leftBox.Grow(aabb);
                        leftCount++;
                    } else {
                        rightBox.Grow(aabb);
                        rightCount++;
                    }
                }

                const float splitNodeCost = leftBox.Area() * leftCount + rightBox.Area() * rightCount;
                if (splitNodeCost < lowestNodeCost) {
                    lowestNodeCost = splitNodeCost;
                    bestCoord = splitCoord;
                    bestAxis = axis;
                }
            }
        }

        if (parentNodeCost <= lowestNodeCost)
            return false;
        break;
    }
    case (SAH_BINNING): {
        parentNodeCost = bounds.Area() * bounds.count;
        const vec3 lengths = this->bounds.Lengths();
        for (int i = 1; i < BINS; i++) {
            for (int axis = 0; axis < 3; axis++) {
                const float splitCoord = bounds.bmin[axis] + lengths[axis] * (float(i) / float(BINS));
                int leftCount = 0, rightCount = 0;
                AABB leftBox = AABB(vec3(1e34f), vec3(-1e34f));
                AABB rightBox = AABB(vec3(1e34f), vec3(-1e34f));

                for (int idx = 0; idx < bounds.count; idx++) {
                    const auto& aabb = aabbs.at(bvhTree->m_PrimitiveIndices[lFirst + idx]);
                    if (aabb.Centroid()[axis] <= splitCoord) {
                        leftBox.Grow(aabb);
                        leftCount++;
                    } else {
                        rightBox.Grow(aabb);
                        rightCount++;
                    }
                }

                const float splitNodeCost = leftBox.Area() * leftCount + rightBox.Area() * rightCount;
                if (splitNodeCost < lowestNodeCost) {
                    lowestNodeCost = splitNodeCost;
                    bestCoord = splitCoord;
                    bestAxis = axis;
                }
            }
        }

        if (parentNodeCost < lowestNodeCost)
            return false;
        break;
    }
    case (CENTRAL_SPLIT):
    default: {
        bestAxis = bounds.LongestAxis();
        const float& axisMin = bounds.bmin[bestAxis];
        const float& axisMax = bounds.bmax[bestAxis];
        bestCoord = (axisMin + axisMax) / 2.f;
    }
    }

    for (int idx = 0; idx < bounds.count; idx++) {
        const auto& aabb = aabbs.at(bvhTree->m_PrimitiveIndices[lFirst + idx]);

        if (aabb.Centroid()[bestAxis] <= bestCoord) // is on left side
        {
            std::swap(bvhTree->m_PrimitiveIndices[lFirst + idx],
                bvhTree->m_PrimitiveIndices[lFirst + lCount]);
            lCount++;
            rFirst++;
            rCount--;
        }
    }

    bvhTree->m_PoolPtrMutex.lock();
    if (bvhTree->m_PoolPtr + 2 >= (aabbs.size() * 2)) {
        bvhTree->m_PoolPtrMutex.unlock();
        return false;
    }

    left = bvhTree->m_PoolPtr++;
    right = bvhTree->m_PoolPtr++;
    bvhTree->m_PoolPtrMutex.unlock();

    bvhTree->m_BVHPool[left].bounds.leftFirst = lFirst;
    bvhTree->m_BVHPool[left].bounds.count = lCount;
    bvhTree->m_BVHPool[right].bounds.leftFirst = rFirst;
    bvhTree->m_BVHPool[right].bounds.count = rCount;

    return true;
}

void bvh::BVHNode::CalculateBounds(
    const std::vector<AABB>& aabbs,
    const std::vector<unsigned int>& primitiveIndices)
{
    auto newBounds = AABB(vec3(1e34f), vec3(-1e34f));
    for (int idx = 0; idx < bounds.count; idx++) {
        newBounds.Grow(aabbs.at(primitiveIndices[bounds.leftFirst + idx]));
    }

    bounds.bmax[0] = newBounds.bmax[0];
    bounds.bmax[1] = newBounds.bmax[1];
    bounds.bmax[2] = newBounds.bmax[2];

    bounds.bmin[0] = newBounds.bmin[0];
    bounds.bmin[1] = newBounds.bmin[1];
    bounds.bmin[2] = newBounds.bmin[2];
}
void bvh::BVHNode::SubdivideMT(
    const std::vector<AABB>& aabbs,
    StaticBVHTree* bvhTree,
    unsigned int depth)
{
    depth++;
    if (GetCount() < MAX_PRIMS || depth >= MAX_DEPTH)
        return; // This is a leaf node

    int left = -1;
    int right = -1;

    if (!Partition(aabbs, bvhTree, left, right))
        return;

    this->bounds.leftFirst = left; // set pointer to children
    this->bounds.count = -1; // no primitives since we are no leaf node

    auto* leftNode = &bvhTree->m_BVHPool[left];
    auto* rightNode = &bvhTree->m_BVHPool[right];

    const auto subLeft = leftNode->GetCount() > 0;
    const auto subRight = rightNode->GetCount() > 0;

    // Start an extra build thread if nr_threads < nr_cores on this machine
    if (!bvhTree->m_ThreadLimitReached) {
        bvhTree->m_ThreadMutex.lock();

        // Only actually create the thread if we have to subdivide both nodes
        if (subLeft && subRight)
            bvhTree->m_BuildingThreads++;

        // Check if we have reached our thread limit
        if (bvhTree->m_BuildingThreads > ctpl::nr_of_cores)
            bvhTree->m_ThreadLimitReached = true;

        bvhTree->m_ThreadMutex.unlock();

        auto leftThread = bvhTree->m_ThreadPool->push([aabbs, bvhTree, depth, leftNode](int) -> void {
            leftNode->CalculateBounds(aabbs, bvhTree->m_PrimitiveIndices);
            leftNode->SubdivideMT(aabbs, bvhTree, depth);
        });

        rightNode->CalculateBounds(aabbs, bvhTree->m_PrimitiveIndices);
        rightNode->SubdivideMT(aabbs, bvhTree, depth);
        leftThread.get();
    } else {
        if (subLeft) {
            leftNode->CalculateBounds(aabbs, bvhTree->m_PrimitiveIndices);
            leftNode->Subdivide(aabbs, bvhTree, depth);
        }

        if (subRight) {
            rightNode->CalculateBounds(aabbs, bvhTree->m_PrimitiveIndices);
            rightNode->Subdivide(aabbs, bvhTree, depth);
        }
    }
}

void bvh::BVHNode::Subdivide(
    const std::vector<AABB>& aabbs,
    std::vector<bvh::BVHNode>& bvhTree,
    std::vector<unsigned int>& primIndices,
    unsigned int depth)
{
    depth++;
    if (GetCount() < MAX_PRIMS || depth >= MAX_DEPTH)
        return; // this is a leaf node

    int left = -1;
    int right = -1;

    if (!Partition(&aabbs, &bvhTree, &primIndices, left, right)) {
        return;
    }

    this->bounds.leftFirst = left; // set pointer to children
    this->bounds.count = -1; // no primitives since we are no leaf node

    auto& leftNode = bvhTree[left];
    auto& rightNode = bvhTree[right];

    if (leftNode.bounds.count > 0) {
        leftNode.CalculateBounds(aabbs, primIndices);
        leftNode.Subdivide(aabbs, bvhTree, primIndices, depth);
    }

    if (rightNode.bounds.count > 0) {
        rightNode.CalculateBounds(aabbs, primIndices);
        rightNode.Subdivide(aabbs, bvhTree, primIndices, depth);
    }
}

void bvh::BVHNode::SubdivideMT(
    const std::vector<AABB>* aabbs,
    std::vector<bvh::BVHNode>* bvhTree,
    std::vector<unsigned int>* primIndices,
    ctpl::ThreadPool* tPool,
    std::mutex* threadMutex,
    std::mutex* partitionMutex,
    unsigned int* threadCount,
    unsigned int depth)
{
    depth++;
    if (GetCount() < MAX_PRIMS || depth >= MAX_DEPTH)
        return; // this is a leaf node

    int left = -1;
    int right = -1;

    if (!Partition(aabbs, bvhTree, primIndices, partitionMutex, left, right))
        return;

    this->bounds.leftFirst = left; // set pointer to children
    this->bounds.count = -1; // no primitives since we are no leaf node

    auto* leftNode = &bvhTree->at(left);
    auto* rightNode = &bvhTree->at(right);

    const auto subLeft = leftNode->GetCount() > 0;
    const auto subRight = rightNode->GetCount() > 0;

    if ((*threadCount) < ctpl::nr_of_cores) {
        threadMutex->lock();

        if (subLeft && subRight)
            (*threadCount)++;

        threadMutex->unlock();

        auto leftThread = tPool->push([aabbs, bvhTree, primIndices, tPool, threadMutex, partitionMutex, threadCount, depth, leftNode](int) -> void {
            leftNode->CalculateBounds(*aabbs, *primIndices);
            leftNode->SubdivideMT(aabbs, bvhTree, primIndices, tPool, threadMutex, partitionMutex, threadCount, depth);
        });

        rightNode->CalculateBounds(*aabbs, *primIndices);
        rightNode->SubdivideMT(aabbs, bvhTree, primIndices, tPool, threadMutex, partitionMutex, threadCount, depth);
        leftThread.get();
    } else {
        if (subLeft) {
            leftNode->CalculateBounds(*aabbs, *primIndices);
            leftNode->Subdivide(*aabbs, *bvhTree, *primIndices, depth);
        }

        if (subRight) {
            rightNode->CalculateBounds(*aabbs, *primIndices);
            rightNode->Subdivide(*aabbs, *bvhTree, *primIndices, depth);
        }
    }
}

bool bvh::BVHNode::Partition(
    const std::vector<AABB>* aabbs,
    std::vector<bvh::BVHNode>* bvhTree,
    std::vector<unsigned int>* primIndices,
    std::mutex* partitionMutex,
    int& left,
    int& right)
{
    const int lFirst = bounds.leftFirst;
    int lCount = 0;
    int rFirst = bounds.leftFirst;
    int rCount = bounds.count;

    float parentNodeCost{}, lowestNodeCost = 1e34f, bestCoord{};
    int bestAxis{};

    parentNodeCost = bounds.Area() * bounds.count;
    const vec3 lengths = this->bounds.Lengths();
    for (int i = 1; i < BINS; i++) {
        const auto binOffset = float(i) / float(BINS);
        for (int axis = 0; axis < 3; axis++) {
            const float splitCoord = bounds.bmin[axis] + lengths[axis] * binOffset;
            int leftCount = 0, rightCount = 0;
            AABB leftBox = { vec3(1e34f), vec3(-1e34f) };
            AABB rightBox = { vec3(1e34f), vec3(-1e34f) };

            for (int idx = 0; idx < bounds.count; idx++) {
                const auto& aabb = aabbs->at(primIndices->at(lFirst + idx));
                if (aabb.Centroid()[axis] <= splitCoord) {
                    leftBox.Grow(aabb);
                    leftCount++;
                } else {
                    rightBox.Grow(aabb);
                    rightCount++;
                }
            }

            const float splitNodeCost = leftBox.Area() * leftCount + rightBox.Area() * rightCount;
            if (splitNodeCost < lowestNodeCost) {
                lowestNodeCost = splitNodeCost;
                bestCoord = splitCoord;
                bestAxis = axis;
            }
        }
    }

    if (parentNodeCost < lowestNodeCost)
        return false;

    for (int idx = 0; idx < bounds.count; idx++) {
        const auto& aabb = aabbs->at(primIndices->at(lFirst + idx));

        if (aabb.Centroid()[bestAxis] <= bestCoord) // is on left side
        {
            std::swap(primIndices->at(lFirst + idx),
                primIndices->at(lFirst + lCount));
            lCount++;
            rFirst++;
            rCount--;
        }
    }

    partitionMutex->lock();
    left = static_cast<int>(bvhTree->size());
    bvhTree->push_back({});
    right = static_cast<int>(bvhTree->size());
    bvhTree->push_back({});
    partitionMutex->unlock();

    bvhTree->at(left).bounds.leftFirst = lFirst;
    bvhTree->at(left).bounds.count = lCount;
    bvhTree->at(right).bounds.leftFirst = rFirst;
    bvhTree->at(right).bounds.count = rCount;

    return true;
}
bool bvh::BVHNode::Partition(const std::vector<AABB>* aabbs, std::vector<bvh::BVHNode>* bvhTree, std::vector<unsigned int>* primIndices, int& left, int& right)
{
    const int lFirst = bounds.leftFirst;
    int lCount = 0;
    int rFirst = bounds.leftFirst;
    int rCount = bounds.count;

    float parentNodeCost{}, lowestNodeCost = 1e34f, bestCoord{};
    int bestAxis{};

    parentNodeCost = bounds.Area() * bounds.count;
    const vec3 lengths = this->bounds.Lengths();
    for (int i = 1; i < BINS; i++) {
        const auto binOffset = float(i) / float(BINS);
        for (int axis = 0; axis < 3; axis++) {
            const float splitCoord = bounds.bmin[axis] + lengths[axis] * binOffset;
            int leftCount = 0, rightCount = 0;
            AABB leftBox = { vec3(1e34f), vec3(-1e34f) };
            AABB rightBox = { vec3(1e34f), vec3(-1e34f) };

            for (int idx = 0; idx < bounds.count; idx++) {
                const auto& aabb = aabbs->at(primIndices->at(lFirst + idx));
                if (aabb.Centroid()[axis] <= splitCoord) {
                    leftBox.Grow(aabb);
                    leftCount++;
                } else {
                    rightBox.Grow(aabb);
                    rightCount++;
                }
            }

            const float splitNodeCost = leftBox.Area() * leftCount + rightBox.Area() * rightCount;
            if (splitNodeCost < lowestNodeCost) {
                lowestNodeCost = splitNodeCost;
                bestCoord = splitCoord;
                bestAxis = axis;
            }
        }
    }

    if (parentNodeCost < lowestNodeCost)
        return false;

    for (int idx = 0; idx < bounds.count; idx++) {
        const auto& aabb = aabbs->at(primIndices->at(lFirst + idx));

        if (aabb.Centroid()[bestAxis] <= bestCoord) // is on left side
        {
            std::swap(primIndices->at(lFirst + idx),
                primIndices->at(lFirst + lCount));
            lCount++;
            rFirst++;
            rCount--;
        }
    }

    left = static_cast<int>(bvhTree->size());
    bvhTree->push_back({});
    right = static_cast<int>(bvhTree->size());
    bvhTree->push_back({});

    bvhTree->at(left).bounds.leftFirst = lFirst;
    bvhTree->at(left).bounds.count = lCount;
    bvhTree->at(right).bounds.leftFirst = rFirst;
    bvhTree->at(right).bounds.count = rCount;

    return true;
}

void bvh::BVHNode::CalculateBounds(int leftChildIndex, const std::vector<BVHNode>& pool)
{
    AABB newBounds = AABB(vec3(1e34f), vec3(-1e34f));
    auto& left = pool[leftChildIndex];
    auto& right = pool[leftChildIndex + 1];
    newBounds.Grow(left.bounds);
    newBounds.Grow(right.bounds);
    bounds.GrowSafe(newBounds);
}

void bvh::BVHNode::CalculateBounds(const std::vector<bvh::GameObjectNode>& objectList, const std::vector<unsigned int>& primIndices)
{
    AABB newBounds = { vec3(1e34f), vec3(-1e34f) };
    for (int idx = 0; idx < bounds.count; idx++) {
        const uint primIdx = primIndices[bounds.leftFirst + idx];
        const GameObjectNode& obj = objectList[primIdx];
        newBounds.Grow(obj.boundsWorldSpace);
    }

    bounds.GrowSafe(newBounds);
}
