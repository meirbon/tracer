#include "Plane.h"
#include "Materials/MaterialManager.h"
#include "Primitives/GpuTriangleList.h"

namespace prims
{
Plane::Plane(vec3 normal, float offset, uint matIdx, vec3 dimMin, vec3 dimMax)
{
	this->m_Normal = normalize(normal);
	vec3 helpNormal;
	if (fabs(m_Normal.x) > fabs(m_Normal.y))
	{
		helpNormal = glm::vec3(m_Normal.z, 0.f, -m_Normal.x) / sqrtf(m_Normal.x * m_Normal.x + m_Normal.z * m_Normal.z);
	}
	else
	{
		helpNormal = glm::vec3(0.f, -m_Normal.z, m_Normal.y) / sqrtf(m_Normal.y * m_Normal.y + m_Normal.z * m_Normal.z);
	}
	this->m_Right = normalize(cross(normal, helpNormal));
	this->m_Forward = normalize(cross(m_Right, m_Normal));

	this->offset = offset;
	this->materialIdx = matIdx;

	const vec3 oset = normal * -offset;
	this->dimMax = dimMax + normal * oset + EPSILON;
	this->dimMin = dimMin + normal * oset - EPSILON;
	this->centroid = (dimMax - dimMin) * .5f + oset;
}

Plane::Plane(glm::vec3 normal, glm::vec3 point)
{
	vec3 NormalizedNormal = normalize(normal);
	this->m_Normal = NormalizedNormal;
	this->offset = -dot(point, NormalizedNormal);
}

void Plane::Intersect(core::Ray &r) const
{
	const float div = dot(m_Normal, r.direction);
	const float t = -(dot(m_Normal, r.origin) + offset) / div;

	if (t < EPSILON) // Intersection is behind the ray
		return;

	const vec3 p = r.GetHitpoint();

	if (p.x < dimMin.x || p.z < dimMin.z || p.x > dimMax.x || p.z > dimMax.z || p.y < dimMin.y || p.y > dimMax.y)
		return;

	if (r.t > t)
	{
		r.t = t;
		r.obj = this;
	}
}

bvh::AABB Plane::GetBounds() const
{
	const vec3 oset = m_Normal * offset;
	return {dimMin - oset - EPSILON, dimMax + oset + EPSILON};
}

vec3 Plane::GetRandomPointOnSurface(const vec3 &direction, vec3 &lNormal, RandomGenerator &rng) const
{
	const float r1 = rng.Rand(1.f);
	const float r2 = rng.Rand(1.f);
	lNormal = dot(direction, m_Normal) > 0 ? -m_Normal : m_Normal;
	return centroid + r1 * dimMin + r2 * dimMax;
}

glm::vec2 Plane::GetTexCoords(const glm::vec3 &hitPoint) const
{
	const vec3 centerToHit = hitPoint - centroid;
	const float dotRight = dot(centerToHit, m_Right);
	const float dotForward = dot(centerToHit, m_Forward);
	float u = fmod(dotRight, 1.f);
	float v = fmod(dotForward, 1.f);
	if (u < 0)
		u = 1.f + u;
	if (v < 0)
		v = 1.f + v;
	return vec2(u, v);
}

glm::vec3 Plane::GetNormal(const glm::vec3 &hitPoint) const { return m_Normal; }

/**
 * p0 is top-right, p1 = top-left, p2 = bottom-right
 */
TrianglePlane::TrianglePlane(vec3 topRight, vec3 topLeft, vec3 bottomRight, uint matIndex, SceneObjectList *objectList)
{
	// find top-left point
	const float minX = fmin(topRight.x, fmin(topLeft.x, bottomRight.x));
	const float minY = fmin(topRight.y, fmin(topLeft.y, bottomRight.y));
	const float minZ = fmin(topRight.z, fmin(topLeft.z, bottomRight.z));

	// invert it to find lower-right point
	const vec3 p3 = vec3(minX, minY, minZ);

	auto *t1 = new Triangle(topRight, topLeft, bottomRight, matIndex);
	auto *t2 = new Triangle(bottomRight, topLeft, p3, matIndex);
	const auto m = MaterialManager::GetInstance()->GetMaterial(matIndex);

	materialIdx = matIndex;
	m_Normal = -normalize(cross(topLeft - topRight, bottomRight - topRight));

	this->m_T1 = t1;
	this->m_T2 = t2;

	if (m.IsLight())
	{
		objectList->AddLight(t1);
		objectList->AddLight(t2);
	}
	else
	{
		objectList->AddObject(t1);
		objectList->AddObject(t2);
	}
}

TrianglePlane::TrianglePlane(vec3 topRight, vec3 topLeft, vec3 bottomRight, uint matIndex, GpuTriangleList *objectList)
{
	// find top-left point
	const float minX = fmin(topRight.x, fmin(topLeft.x, bottomRight.x));
	const float minY = fmin(topRight.y, fmin(topLeft.y, bottomRight.y));
	const float minZ = fmin(topRight.z, fmin(topLeft.z, bottomRight.z));

	const auto &m = MaterialManager::GetInstance()->GetMaterial(matIndex);

	// invert it to find lower-right point
	const vec3 bottomLeft = vec3(minX, minY, minZ);

	materialIdx = matIndex;
	m_Normal = -normalize(cross(topLeft - topRight, bottomRight - topRight));

	if (m.IsLight())
	{
		objectList->AddLight(GpuTriangle(topRight, topLeft, bottomRight, m_Normal, m_Normal, m_Normal, matIndex));
		objectList->AddLight(GpuTriangle(bottomRight, topLeft, bottomLeft, m_Normal, m_Normal, m_Normal, matIndex));
	}
	else
	{
		objectList->AddTriangle(GpuTriangle(topRight, topLeft, bottomRight, m_Normal, m_Normal, m_Normal, matIndex));
		objectList->AddTriangle(GpuTriangle(bottomRight, topLeft, bottomLeft, m_Normal, m_Normal, m_Normal, matIndex));
	}
}

void TrianglePlane::Intersect(core::Ray &r) const
{
	m_T1->Intersect(r);
	m_T2->Intersect(r);
}

bvh::AABB TrianglePlane::GetBounds() const
{
	bvh::AABB bounds = m_T1->GetBounds();
	bounds.Grow(m_T2->GetBounds());
	return bounds;
}

glm::vec3 TrianglePlane::GetRandomPointOnSurface(const glm::vec3 &direction, glm::vec3 &lNormal,
												 RandomGenerator &rng) const
{
	if (rng.Rand(1.f) <= .5f)
	{
		return m_T1->GetRandomPointOnSurface(direction, lNormal, rng);
	}
	else
	{
		return m_T2->GetRandomPointOnSurface(direction, lNormal, rng);
	}
}

glm::vec3 TrianglePlane::GetNormal(const glm::vec3 &hitPoint) const { return m_Normal; }

glm::vec2 TrianglePlane::GetTexCoords(const glm::vec3 &hitPoint) const
{
	auto t0 = m_T1->GetTexCoords(hitPoint);
	auto t1 = m_T2->GetTexCoords(hitPoint);

	if (t0.x < 0.0f || t0.x > 1.0f || t0.y < 0.0f || t0.y > 1.0f)
		return t1;

	return t0;
}

void TrianglePlane::create(vec3 topRight, vec3 topLeft, vec3 bottomRight, uint matIndex, TriangleList *tList)
{
	// find top-left point
	const float minX = fmin(topRight.x, fmin(topLeft.x, bottomRight.x));
	const float minY = fmin(topRight.y, fmin(topLeft.y, bottomRight.y));
	const float minZ = fmin(topRight.z, fmin(topLeft.z, bottomRight.z));

	// invert it to find lower-right point
	const vec3 p3 = vec3(minX, minY, minZ);

	const auto normal = -normalize(cross(topLeft - topRight, bottomRight - topRight));
	tList->addTriangle(topRight, topLeft, bottomRight, normal, normal, normal, matIndex, vec2(0, 0), vec2(1, 0),
					   vec2(0, 1));
	tList->addTriangle(bottomRight, topLeft, p3, normal, normal, normal, matIndex, vec2(0, 1), vec2(1, 0), vec2(1, 1));
}
} // namespace prims