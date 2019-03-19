#pragma once

#include "Core/Ray.h"
#include "Materials/Material.h"
#include "SceneObject.h"
#include "SceneObjectList.h"
#include "Triangle.h"

namespace prims
{
struct GpuTriangle;
class GpuTriangleList;

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
};

class TrianglePlane : public SceneObject
{
  public:
	TrianglePlane() = default;
	/**
	 * p0 is top-right, p1 = top-left, p2 = bottom-left, p3 = bottom-right
	 */
	TrianglePlane(glm::vec3 topRight, glm::vec3 topLeft, glm::vec3 bottomRight, uint matIndex,
				  SceneObjectList *objectList);
	TrianglePlane(glm::vec3 topRight, glm::vec3 topLeft, glm::vec3 bottomRight, uint matIndex,
				  GpuTriangleList *objectList);
	~TrianglePlane() override = default;

	glm::vec3 m_Normal{};
	Triangle *m_T1{};
	Triangle *m_T2{};

	void Intersect(core::Ray &r) const override;
	bvh::AABB GetBounds() const override;
	glm::vec3 GetRandomPointOnSurface(const glm::vec3 &direction, glm::vec3 &lNormal,
									  RandomGenerator &rng) const override;
	glm::vec3 GetNormal(const glm::vec3 &hitPoint) const override;
	glm::vec2 GetTexCoords(const glm::vec3 &hitPoint) const override;
};
} // namespace prims