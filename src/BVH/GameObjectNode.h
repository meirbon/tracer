#pragma once

#include <vector>

#include "Core/Ray.h"
#include "BVH/AABB.h"
#include "Primitives/SceneObject.h"

namespace bvh
{
class GameObject;

class GameObjectNode
{
  public:
	GameObjectNode(glm::mat4 transformationMat, glm::mat4 inverseMat, bvh::GameObject *gameObject);

	bool Intersect(const core::Ray &r) const;
	bool Intersect(const core::Ray &r, float &tnear, float &tfar) const;

	void Traverse(core::Ray &r) const;
	unsigned int TraverseDebug(core::Ray &r) const;
	void TraverseShadow(core::Ray &r) const;

	glm::mat4 transformationMat;
	glm::mat4 inverseMat;
	bvh::GameObject *gameObject;
	bvh::AABB boundsWorldSpace;

	inline void SetCount(const int value) { boundsWorldSpace.count = value; }

	inline void SetLeftFirst(const unsigned int value) { boundsWorldSpace.leftFirst = value; }

	inline int GetCount() const { return boundsWorldSpace.count; }

	inline int GetLeftFirst() const { return boundsWorldSpace.leftFirst; }

	void ConstructWorldBounds();
};
} // namespace bvh