#include "Plane.h"
#include "Materials/MaterialManager.h"

namespace prims
{
Plane::Plane(vec3 normal, float offset, uint matIdx, vec3 dimMin, vec3 dimMax)
{
	this->m_Normal = normalize(normal);
	vec3 helpNormal;
	if (fabs(m_Normal.x) > fabs(m_Normal.y))
		helpNormal = glm::vec3(m_Normal.z, 0.f, -m_Normal.x) / sqrtf(m_Normal.x * m_Normal.x + m_Normal.z * m_Normal.z);
	else
		helpNormal = glm::vec3(0.f, -m_Normal.z, m_Normal.y) / sqrtf(m_Normal.y * m_Normal.y + m_Normal.z * m_Normal.z);

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

glm::vec3 Plane::GetNormal(const glm::vec3 &) const { return m_Normal; }

/**
 * p0 is top-right, p1 = top-left, p2 = bottom-right
 */
std::tuple<Triangle *, Triangle *> Plane::create(vec3 topRight, vec3 topLeft, vec3 bottomRight, uint matIndex,
												 SceneObjectList *objectList)
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

	if (m.type == Light)
	{
		objectList->AddLight(t1);
		objectList->AddLight(t2);
	}
	else
	{
		objectList->AddObject(t1);
		objectList->AddObject(t2);
	}

	return std::make_tuple(t1, t2);
}

void Plane::create(vec3 topRight, vec3 topLeft, vec3 bottomRight, uint matIndex, TriangleList *tList, bool flipNormal)
{
	// find top-left point
	const vec3 p3 = min(topRight, min(topLeft, bottomRight));

	const auto normal = (flipNormal ? -1.0f : 1.0f) * normalize(cross(topLeft - topRight, bottomRight - topRight));
	tList->addTriangle(topRight, topLeft, bottomRight, normal, normal, normal, matIndex, vec2(0, 0), vec2(1, 0),
					   vec2(0, 1));
	tList->addTriangle(bottomRight, topLeft, p3, normal, normal, normal, matIndex, vec2(0, 1), vec2(1, 0), vec2(1, 1));
}
} // namespace prims