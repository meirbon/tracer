#include "BVH/TopLevelBVH.h"
#include "BVH/GameObjectNode.h"
#include "Renderer/Renderer.h"

namespace bvh
{
TopLevelBVH::TopLevelBVH(SceneObjectList *staticObjectList, std::vector<GameObject *> *gObjectList, BVHType type,
                         ctpl::ThreadPool *tPool)
{
    this->m_Type = type;
    this->m_ThreadPool = tPool;
    this->m_StaticObjectList = staticObjectList;
    this->gameObjectList = gObjectList;

    FlattenGameObjects();
    ConstructBVH();
}

TopLevelBVH::TopLevelBVH(WorldScene *staticTree, SceneObjectList *staticObjectList,
                         std::vector<GameObject *> *gObjectList, BVHType type, ctpl::ThreadPool *tPool)
{
    this->m_Type = type;
    this->m_ThreadPool = tPool;
    this->m_StaticBVHTree = staticTree;
    this->m_StaticObjectList = staticObjectList;
    this->gameObjectList = gObjectList;

    FlattenGameObjects();
    ConstructBVH();
}

const std::vector<SceneObject *> &TopLevelBVH::GetLights() const { return m_StaticObjectList->GetLights(); }

void TopLevelBVH::TraceRay(Ray &r) const
{
    m_StaticBVHTree->TraceRay(r);

    if (m_DynamicNodes.empty())
        return;

    IntersectDynamic(r);
}

bool TopLevelBVH::TraceShadowRay(Ray &r, float tMax) const
{
    m_StaticBVHTree->TraceShadowRay(r, tMax);

    if (m_DynamicNodes.empty())
        return r.t < tMax;

    IntersectDynamic(r);

    TraceRay(r);
    return r.t < tMax;
}

TopLevelBVH::~TopLevelBVH()
{
    delete m_StaticBVHTree;
    for (auto *obj : *gameObjectList)
    {
        delete obj;
    }

    delete gameObjectList;
}

void TopLevelBVH::IntersectDynamic(Ray &r) const
{
    if (CanUseDynamicBVH[m_DynamicTreeIndex])
    {
        const auto &dTree = m_DynamicBVHTree[m_DynamicTreeIndex];
        const auto &dInidices = m_DynamicIndices[m_DynamicTreeIndex];

        if (dTree[0].Intersect(r))
            dTree[0].Traverse(r, m_DynamicNodes, dTree, dInidices);
        return; // Early out
    }

    // Else loop over game objects
    for (const auto &m_DynamicNode : m_DynamicNodes)
    {
        if (m_DynamicNode.Intersect(r))
            m_DynamicNode.Traverse(r);
    }
}

void TopLevelBVH::IntersectDynamicShadow(Ray &r) const
{
    if (CanUseDynamicBVH[m_DynamicTreeIndex])
    {
        const auto &dTree = m_DynamicBVHTree[m_DynamicTreeIndex];
        const auto &dInidices = m_DynamicIndices[m_DynamicTreeIndex];

        if (dTree[0].Intersect(r))
            dTree[0].Traverse(r, m_DynamicNodes, dTree, dInidices);
        return; // Early out
    }

    // Else loop over game objects
    for (const auto &m_DynamicNode : m_DynamicNodes)
    {
        if (m_DynamicNode.Intersect(r))
            m_DynamicNode.Traverse(r);
    }
}

void TopLevelBVH::UpdateDynamic(Renderer &renderer)
{
    if (m_DynamicNodes.empty())
        return;

    FlattenGameObjects();

    BVHNode *root = &m_DynamicBVHTree[m_DynamicTreeIndex][0];
    UpdateDynamic(root);
    renderer.Reset();
}

void TopLevelBVH::UpdateDynamic(BVHNode *caller)
{
    if (caller->IsLeaf())
    {
        caller->CalculateBounds(m_DynamicNodes, m_DynamicIndices[m_DynamicTreeIndex]);
    }
    else
    {
        int leftIndex = caller->GetLeftFirst();
        auto *left = &m_DynamicBVHTree[m_DynamicTreeIndex][leftIndex];
        auto *right = &m_DynamicBVHTree[m_DynamicTreeIndex][leftIndex + 1];

        auto l = m_ThreadPool->push([left, this](int) -> void { UpdateDynamic(left); });
        UpdateDynamic(right);
        l.get();

        caller->CalculateBounds(leftIndex, m_DynamicBVHTree[m_DynamicTreeIndex]);
    }
}

unsigned int TopLevelBVH::TraceDebug(Ray &r) const
{
    unsigned int depth = 0;
    depth += m_StaticBVHTree->TraceDebug(r);
    if (CanUseDynamicBVH[m_DynamicTreeIndex])
    {
        const auto &dTree = m_DynamicBVHTree[m_DynamicTreeIndex];
        const auto &dInidices = m_DynamicIndices[m_DynamicTreeIndex];

        if (dTree[0].Intersect(r))
            depth += dTree[0].TraverseDebug(r, m_DynamicNodes, dTree, dInidices);
        return depth;
    }

    // Else loop over game objects
    for (const auto &m_DynamicNode : m_DynamicNodes)
    {
        if (m_DynamicNode.Intersect(r))
            depth += m_DynamicNode.TraverseDebug(r);
    }

    return depth;
}

void TopLevelBVH::SwapDynamicTrees() { m_DynamicTreeIndex = 1 - m_DynamicTreeIndex; }

int TopLevelBVH::GetActiveDynamicTreeIndex() { return m_DynamicTreeIndex; }

int TopLevelBVH::GetInActiveDynamicTreeIndex() { return 1 - m_DynamicTreeIndex; }

void TopLevelBVH::ConstructBVH()
{
    auto dynamicBuild = m_ThreadPool->push([this](int) {
        FlattenGameObjects();
        ConstructDynamicBVH(m_DynamicTreeIndex);
    });

    if (!this->m_StaticBVHTree)
        this->m_StaticBVHTree = new MBVHTree(m_StaticObjectList, this->m_Type, this->m_ThreadPool);

    dynamicBuild.get();
}

AABB TopLevelBVH::GetNodeBounds(unsigned int index) { return {}; }

unsigned int TopLevelBVH::GetPrimitiveCount()
{
    unsigned int c = 0;
    for (auto &n : m_DynamicNodes)
    {
        c += n.gameObject->m_BVHTree->GetPrimitiveCount();
    }

    return m_StaticBVHTree->GetPrimitiveCount() + c;
}

std::future<void> TopLevelBVH::ConstructNewDynamicBVHParallel(int newIndex)
{
    return m_ThreadPool->push([this, newIndex](int) { ConstructDynamicBVH(newIndex); });
}

void TopLevelBVH::ConstructDynamicBVH(int newIndex)
{
    if (m_DynamicNodes.empty())
    {
        return;
    }

    Timer t;
    CanUseDynamicBVH[newIndex] = false;
    m_DynamicIndices[newIndex].clear();
    m_DynamicBVHTree[newIndex].clear();
    m_DynamicIndices[newIndex].clear();

    std::vector<bvh::AABB> aabbs;
    for (unsigned int i = 0; i < m_DynamicNodes.size(); i++)
    {
        m_DynamicIndices[newIndex].push_back(i);
        //        m_DynamicNodes[i].ConstructWorldBounds();
        aabbs.push_back(m_DynamicNodes[i].boundsWorldSpace);
    }

    BVHNode rootNode = {};
    rootNode.bounds.leftFirst = 0;
    rootNode.bounds.count = int(m_DynamicNodes.size());
    rootNode.CalculateBounds(aabbs, m_DynamicIndices[newIndex]);
    m_DynamicBVHTree[newIndex].push_back(rootNode);

    if (m_ThreadPool != nullptr)
    {
        unsigned int threadCount = 0;
        rootNode.SubdivideMT(&aabbs, &m_DynamicBVHTree[newIndex], &m_DynamicIndices[newIndex], m_ThreadPool,
                             &m_ThreadMutex, &m_PartitionMutex, &threadCount, 1);
    }
    else
    {
        rootNode.Subdivide(aabbs, m_DynamicBVHTree[newIndex], m_DynamicIndices[newIndex], 1);
    }

    CanUseDynamicBVH[newIndex] = true;
    std::cout << "Building dynamic BVH took: " << t.elapsed() << "ms." << std::endl;
}

void TopLevelBVH::FlattenGameObjects(GameObject *currentObject, mat4 currentTransform, mat4 currentInverse,
                                     std::vector<GameObjectNode> &dNodes)
{
    if (currentObject->IsLeaf())
    {
        dNodes.emplace_back(currentObject->m_TransformationMat * currentTransform,
                            currentInverse * currentObject->m_InverseMat, currentObject);
    }
    else
    {
        for (GameObject *gameObject : *currentObject->m_Children)
        {
            FlattenGameObjects(gameObject, gameObject->m_TransformationMat * currentTransform,
                               currentInverse * gameObject->m_InverseMat, dNodes);
        }
    }
}

void TopLevelBVH::FlattenGameObjects()
{
    std::vector<GameObjectNode> nodes;
    for (GameObject *gameObject : *gameObjectList)
    {
        FlattenGameObjects(gameObject, gameObject->m_TransformationMat, gameObject->m_InverseMat, nodes);
    }
    m_DynamicNodes = nodes;
}

void TopLevelBVH::IntersectDynamicWithStack(Ray &r) const
{
    if (m_DynamicNodes.empty())
    {
        return;
    }

    int stackptr = 0;
    BVHTraversal todo[64];
    float t1near, t1far;
    float t2near, t2far;
    const auto &objects = m_DynamicNodes;

    if (!m_DynamicBVHTree[m_DynamicTreeIndex][0].Intersect(r))
    {
        return;
    }

    // "Push" on the root node to the working set
    todo[stackptr].nodeIdx = 0;

    while (stackptr >= 0)
    {
        // Pop off the next node to work on.
        const unsigned int &nodeIdx = todo[stackptr].nodeIdx;
        const float &tNear = todo[stackptr].tNear;
        const auto &node = m_DynamicBVHTree[m_DynamicTreeIndex][nodeIdx];
        stackptr--;

        int leftFirst = node.bounds.leftFirst;
        int right = leftFirst + 1;

        // If this node is further than the closest found intersection, continue
        if (tNear > r.t)
            continue;

        if (node.IsLeaf())
        { // Leaf node
            for (int idx = node.bounds.leftFirst; idx < node.bounds.leftFirst + node.bounds.count; idx++)
            {
                objects[m_DynamicIndices[m_DynamicTreeIndex][idx]].Traverse(r);
            }
        }
        else
        { // Not a leaf
            const bool hitLeftNode = m_DynamicBVHTree[m_DynamicTreeIndex][leftFirst].Intersect(r, t1near, t1far);
            const bool hitRightNode = m_DynamicBVHTree[m_DynamicTreeIndex][right].Intersect(r, t2near, t2far);

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
} // namespace bvh