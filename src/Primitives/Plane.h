#pragma once

#include <tuple>

#include "Core/Ray.h"
#include "Materials/Material.h"
#include "Primitives/SceneObject.h"
#include "Primitives/SceneObjectList.h"
#include "Primitives/Triangle.h"
#include "Primitives/TriangleList.h"

namespace prims
{

class Plane : public SceneObject
{
  public:
	Plane() = default;
	Plane(glm::vec3 normal, float offset, uint matIdx, glm::vec3 dimMin, glm::vec3 dimMax);
	Plane(glm::vec3 normal, glm::vec3 point);
	void Intersect(core::Ray &r) const override;
	~Plane() override = default;

	glm::vec3 dimMin{}, dimMax{};
	glm::vec3 m_Normal{};
	glm::vec3 m_Right{};
	glm::vec3 m_Forward{};
	float offset{};

	bvh::AABB GetBounds() const override;
	glm::vec3 GetRandomPointOnSurface(const glm::vec3 &direction, glm::vec3 &lNormal,
									  RandomGenerator &rng) const override;
	glm::vec3 GetNormal(const glm::vec3 &hitPoint) const override;
	glm::vec2 GetTexCoords(const glm::vec3 &hitPoint) const override;

	static std::tuple<Triangle *, Triangle *> create(glm::vec3 topRight, glm::vec3 topLeft, glm::vec3 bottomRight,
													 uint matIndex, SceneObjectList *objectList);
	static void create(vec3 topRight, vec3 topLeft, vec3 bottomRight, uint matIndex, TriangleList *tList,
					   bool flipNormal = false);
};

} // namespace prims