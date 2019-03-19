#include "Primitives/Sphere.h"
#include "Core/Ray.h"
#include "Shared.h"

using namespace glm;

namespace prims
{
Sphere::Sphere(vec3 pos, float radius, unsigned int matIdx)
{
	centroid = pos;
	radiusSquared = radius * radius;
	materialIdx = matIdx;
}

void Sphere::Intersect(core::Ray &r) const
{
	const float a = dot(r.direction, r.direction);
	const vec3 rPos = r.origin - centroid;

	const float b = dot(r.direction * 2.f, rPos);
	const float rPos2 = dot(rPos, rPos);
	const float c = rPos2 - radiusSquared;

	const float discriminant = (b * b) - (4 * a * c);
	if (discriminant < 0.f)
		return;

	const float div2a = 1.f / (2.f * a);
	const float sqrtDis = discriminant > 0.f ? sqrtf(discriminant) : 0.f;

	const float t1 = ((-b) + sqrtDis) * div2a;
	const float t2 = ((-b) - sqrtDis) * div2a;

	const float tSlow = t1 > 0.f && t1 < t2 ? t1 : t2; // Take the largest one, one should be positive...
	if (tSlow <= EPSILON || tSlow > r.t)
		return;

	r.t = tSlow;
	r.obj = this;
}

bvh::AABB Sphere::GetBounds() const
{
	const float radius = sqrtf(radiusSquared) + EPSILON;
	const vec3 min = centroid - radius;
	const vec3 max = centroid + radius;
	return {min, max};
}

vec3 Sphere::GetRandomPointOnSurface(const vec3 &direction, vec3 &lNormal, RandomGenerator &rng) const
{
	const vec3 point = rng.RandomPointOnHemisphere(direction);
	lNormal = point;
	return point * sqrtf(this->radiusSquared) + centroid;
}

glm::vec3 Sphere::GetNormal(const glm::vec3 &hitPoint) const { return normalize(hitPoint - centroid); }

glm::vec2 Sphere::GetTexCoords(const glm::vec3 &hitPoint) const
{
	const vec3 normal = this->GetNormal(hitPoint);
	const float u = atan2f(normal.x, normal.z) / (2 * 3.14159265358979323846f) + .5f;
	const float v = normal.y * .5f + .5f;
	return vec2(u, v);
}
} // namespace prims