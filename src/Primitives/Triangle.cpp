#include "Triangle.h"
#include "Materials/MaterialManager.h"
#include "Primitives/SceneObjectList.h"
#include "Shared.h"

namespace prims
{
#define EPSILON_T 0.000001f

Triangle::Triangle(vec3 p0, vec3 p1, vec3 p2, uint matIndex, vec2 t0, vec2 t1, vec2 t2)
	: p0(p0), p1(p1), p2(p2), t0(t0), t1(t1), t2(t2)
{
	this->materialIdx = matIndex;
	this->normal = normalize(cross(p1 - p0, p2 - p0));
	this->n0 = this->n1 = this->n2 = normal;
	this->centroid = (p0 + p1 + p2) / 3.f;

	// Heron's formula
	const float a = glm::length(p0 - p1);
	const float b = glm::length(p1 - p2);
	const float c = glm::length(p2 - p0);
	const float s = (a + b + c) / 2.f;
	this->m_Area = sqrtf(s * (s - a) * (s - b) * (s - c));
}

Triangle::Triangle(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 n0, glm::vec3 n1, glm::vec3 n2, uint matIndex)
	: p0(p0), p1(p1), p2(p2), n0(n0), n1(n1), n2(n2), t0(vec3(-1.f, -1.f, -1.f))
{
	this->materialIdx = matIndex;
	this->centroid = (p0 + p1 + p2) / 3.f;
	this->normal = normalize(cross(p1 - p0, p2 - p0));
	// Heron's formula
	const float a = glm::length(p0 - p1);
	const float b = glm::length(p1 - p2);
	const float c = glm::length(p2 - p0);
	const float s = (a + b + c) / 2.f;
	this->m_Area = sqrtf(s * (s - a) * (s - b) * (s - c));
}

Triangle::Triangle(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 n0, glm::vec3 n1, glm::vec3 n2, uint matIndex,
				   glm::vec2 t0, glm::vec2 t1, glm::vec2 t2)
	: p0(p0), p1(p1), p2(p2), n0(n0), n1(n1), n2(n2), t0(t0), t1(t1), t2(t2)
{
	this->materialIdx = matIndex;
	this->centroid = (p0 + p1 + p2) / 3.f;
	this->normal = normalize(cross(p1 - p0, p2 - p0));
	// Heron's formula
	const float a = glm::length(p0 - p1);
	const float b = glm::length(p1 - p2);
	const float c = glm::length(p2 - p0);
	const float s = (a + b + c) / 2.f;
	this->m_Area = sqrtf(s * (s - a) * (s - b) * (s - c));
}

Triangle::Triangle(vec3 p0, vec3 p1, vec3 p2, uint matIndex) : p0(p0), p1(p1), p2(p2), t0(vec3(-1.f, -1.f, -1.f))
{
	this->materialIdx = matIndex;
	this->normal = normalize(cross(p1 - p0, p2 - p0));
	this->n0 = this->n1 = this->n2 = normal;
	this->t0 = vec3(-1.f, -1.f, -1.f);
	this->centroid = (p0 + p1 + p2) / 3.f;
	this->normal = normalize(cross(p1 - p0, p2 - p0));
	// Heron's formula
	const float a = glm::length(p0 - p1);
	const float b = glm::length(p1 - p2);
	const float c = glm::length(p2 - p0);
	const float s = (a + b + c) / 2.f;
	this->m_Area = sqrtf(s * (s - a) * (s - b) * (s - c));
}

void Triangle::Intersect(core::Ray &r) const
{
	// https://en.wikipedia.org/wiki/Möller-Trumbore_intersection_algorithm
	const vec3 edge1 = p1 - p0;
	const vec3 edge2 = p2 - p0;

	const vec3 h = cross(r.direction, edge2);

	const float a = dot(edge1, h);
	if (a > -EPSILON_T && a < EPSILON_T)
		return; // This ray is parallel to this triangle.

	const float f = 1.f / a;
	const vec3 s = r.origin - p0;
	const float u = f * dot(s, h);
	if (u < 0 || u > 1)
		return;

	const vec3 q = cross(s, edge1);
	const float v = f * dot(r.direction, q);
	if (v < 0 || u + v > 1)
		return;

	// At this stage we can compute t to find out where the intersection point
	// is on the line.
	const float t = f * dot(edge2, q);

	if (t > EPSILON_T && r.t > t) // ray intersection
	{
		r.t = t;
		r.obj = this;
	}
}

const vec3 Triangle::GetBarycentricCoordinatesAt(const vec3 &p) const
{
	const float areaABC = dot(normal, cross(p1 - p0, p2 - p0));
	const float areaPBC = dot(normal, cross(p1 - p, p2 - p));
	const float areaPCA = dot(normal, cross(p2 - p, p0 - p));

	const float alpha = areaPBC / areaABC;
	const float beta = areaPCA / areaABC;
	const float gamma = 1.f - alpha - beta;

	return vec3(alpha, beta, gamma);
}

bvh::AABB Triangle::GetBounds() const
{
	const float minX = glm::min(glm::min(p0.x, p1.x), p2.x) - EPSILON;
	const float minY = glm::min(glm::min(p0.y, p1.y), p2.y) - EPSILON;
	const float minZ = glm::min(glm::min(p0.z, p1.z), p2.z) - EPSILON;

	const float maxX = glm::max(glm::max(p0.x, p1.x), p2.x) + EPSILON;
	const float maxY = glm::max(glm::max(p0.y, p1.y), p2.y) + EPSILON;
	const float maxZ = glm::max(glm::max(p0.z, p1.z), p2.z) + EPSILON;

	return {vec3(minX, minY, minZ), vec3(maxX, maxY, maxZ)};
}

vec3 Triangle::GetRandomPointOnSurface(const vec3 &, vec3 &lNormal, RandomGenerator &rng) const
{
	lNormal = normal;
	float r1 = rng.Rand(1.f);
	float r2 = rng.Rand(1.f);

	if (r1 + r2 > 1.0f)
	{
		r1 = 1.0f - r1;
		r2 = 1.0f - r2;
	}

	const float a = 1.0f - r1 - r2;
	const float &b = r1;
	const float &c = r2;

	return a * p0 + b * p1 + c * p2;
}

// glm::vec3 Triangle::CalculateLight(
//    const Ray& r,
//    const material::Material* mat,
//    const WorldScene* m_Scene) const
//{
//
//    // TODO: Fix this
//    glm::vec3 ret = glm::vec3(0.f);
//    const vec3 p = r.GetHitpoint();
//    const vec3 towardsLight = this->GetPosition() - p;
//    const vec3 towardsLightNorm = normalize(towardsLight);
//
//    if (dot(this->normal, towardsLightNorm) >= 0) { // object not in light
//        return ret;
//    }
//
//    const vec3 normal = r.obj->GetNormal(p);
//
//    Ray shadowRay = Ray(this->GetPosition(), -towardsLightNorm);
//    m_Scene->TraceRay(shadowRay);
//
//    const vec3 shadowNormal =
//    shadowRay.obj->GetNormal(shadowRay.GetHitpoint()); const float
//    hitDotShadowHit = dot(normal, shadowNormal); if (shadowRay.IsValid() &&
//    (hitDotShadowHit < 0 || dot(r.direction, normal) > 0))
//        return ret;
//
//    const auto lightMat =
//    material::MaterialManager::GetInstance()->GetMaterial(this->materialIdx);
//    const vec3 reflectDir = (r.direction - (2.0f * dot(normal, r.direction) *
//    normal)); const float specular = dot(towardsLightNorm,
//    normalize(reflectDir))
//        * lightMat.GetIntensity()
//        / (dot(towardsLight, towardsLight))
//        * mat->GetSpecular();
//    const glm::vec3 specularColor = specular > 0.f
//        ? lightMat.GetEmission() * powf(specular, mat->shininess)
//        : glm::vec3(0.f, 0.f, 0.f);
//    ret += mat->GetSpecularColor() * specularColor;
//
//    if (dot(normal, shadowNormal) < 0)
//        return ret;
//
//    const float distToLightIntersection = shadowRay.t;
//    const float distToLight = sqrtf(dot(towardsLight, towardsLight));
//
//    if ((distToLightIntersection + EPSILON) > distToLight &&
//    (distToLightIntersection - EPSILON) < distToLight) {
//        const float illum = lightMat.GetIntensity() / (dot(towardsLight,
//        towardsLight)) * mat->GetDiffuse(); const glm::vec3 lightColor = illum
//        > 0.f ? lightMat->GetEmission() * illum *
//        glm::max(dot(towardsLightNorm, normal), 0.f)
//                                                 : glm::vec3(0.0f);
//        ret += mat->GetDiffuseColor(r.obj, p, m_Scene->GetTextures()) *
//        lightColor;
//    }
//
//    return ret;
//}

glm::vec3 Triangle::GetNormal(const glm::vec3 &hitPoint) const
{
	const vec3 bary = GetBarycentricCoordinatesAt(hitPoint);
	return normalize(bary.x * n0 + bary.y * n1 + bary.z * n2);
}

glm::vec2 Triangle::GetTexCoords(const glm::vec3 &hitPoint) const
{
	const vec3 bary = GetBarycentricCoordinatesAt(hitPoint);
	const vec2 texCoords = bary.x * t0 + bary.y * t1 + bary.z * t2;
	float x = fmod(texCoords.x, 1.0f);
	float y = fmod(texCoords.y, 1.0f);

	if (x < 0)
		x += 1.0f;
	if (y < 0)
		y += 1.0f;

	return {x, y};
}

float Triangle::CalculateArea() const
{
	const float a = glm::length(p0 - p1);
	const float b = glm::length(p1 - p2);
	const float c = glm::length(p2 - p0);
	const float s = (a + b + c) / 2.f;
	return sqrtf(s * (s - a) * (s - b) * (s - c));
}
} // namespace prims