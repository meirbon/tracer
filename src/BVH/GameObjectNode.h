#pragma once

#include <vector>

#include "Core/Ray.h"
#include "Primitives/SceneObject.h"
#include "Utils/Template.h"

namespace bvh
{
class GameObject;

class GameObjectNode
{
  public:
    GameObjectNode(glm::mat4 transformationMat, glm::mat4 inverseMat,
                   bvh::GameObject *gameObject);

    bool Intersect(const Ray &r) const;
    bool Intersect(const Ray &r, float &tnear, float &tfar) const;

    void Traverse(Ray &r) const;
    unsigned int TraverseDebug(Ray &r) const;
    void TraverseShadow(Ray &r) const;

    glm::mat4 transformationMat;
    glm::mat4 inverseMat;
    bvh::GameObject *gameObject;
    bvh::AABB boundsWorldSpace;

    inline void SetCount(const int value) { boundsWorldSpace.count = value; }

    inline void SetLeftFirst(const uint value)
    {
        boundsWorldSpace.leftFirst = value;
    }

    inline int GetCount() const { return boundsWorldSpace.count; }

    inline int GetLeftFirst() const { return boundsWorldSpace.leftFirst; }

    void ConstructWorldBounds();
};
} // namespace bvh