#include "LightSpot.h"
#include "BVH/TopLevelBVH.h"
#include "Core/Ray.h"
#include "Materials/MaterialManager.h"

using namespace glm;

namespace prims
{
LightSpot::LightSpot(vec3 position, vec3 direction, float angleInner, float angleOuter, unsigned int matIdx,
					 float objRadius)
{
	this->centroid = position;
	this->Direction = normalize(direction);
	this->cosAngleInner = cos(angleInner);
	this->cosAngleOuter = cos(angleOuter);
	this->materialIdx = matIdx;
	this->m_RadiusSquared = objRadius * objRadius;
	this->m_Area = 4 * PI * objRadius * objRadius * glm::radians(180.f) / (angleInner + angleOuter);
}
//
// glm::vec3 LightSpot::CalculateLight(
//    const Ray& r,
//    const material::Material* mat,
//    const WorldScene* m_Scene) const
//{
//    glm::vec3 ret = glm::vec3(0.f);
//    const vec3 p = r.GetHitpoint();
//    const vec3 towardsLight = this->GetPosition() - p;
//    const float towardsLightDot = dot(towardsLight, towardsLight);
//    const float distToLight = sqrtf(towardsLightDot);
//    const vec3 towardsLightNorm = towardsLight / distToLight;
//
//    vec3 normal = r.obj->GetNormal(p);
//    const float dotRayNormal = dot(r.direction, normal);
//    if (dotRayNormal > 0) {
//        normal = -normal;
//    }
//
//    const float dotNormalTowardsLightNormal = dot(normal, towardsLightNorm);
//    if (dotNormalTowardsLightNormal <= 0) { // object facing away from light
//        return ret;
//    }
//
//    Ray shadowRay = Ray(p + EPSILON * towardsLightNorm, towardsLightNorm);
//    m_Scene->TraceRay(shadowRay);
//
//    if (!shadowRay.IsValid() || shadowRay.obj == this) {
//        const auto lightMat =
//        material::MaterialManager::GetInstance()->GetMaterial(this->materialIdx);
//        float cosA = dot(-towardsLightNorm, Direction);
//
//        const vec3 reflectDir = (r.direction - (2.0f * dot(r.direction,
//        normal) * normal)); float illum = lightMat->GetIntensity() /
//        towardsLightDot;
//
//        if (mat.IsReflective()) {
//            const float specular = dot(towardsLightNorm,
//            normalize(reflectDir)) * illum * mat.GetSpecular(); const
//            glm::vec3 lightColor = lightMat->GetEmission() * powf(specular,
//            mat.shininess); ret += mat->GetSpecularColor() * lightColor;
//        }
//
//        if (cosA >= cosAngleOuter && mat->IsDiffuse()) {
//            float delta = 1.f;
//            if (cosA < cosAngleInner) // We are inside the inner angle
//            {
//                const float del = (cosA - cosAngleOuter) / (cosAngleInner -
//                cosAngleOuter); delta = (del * del) * (del * del);
//            }
//
//            const float power = 2.f * PI * (1.f - .5f * (cosAngleInner +
//            cosAngleOuter)); illum *= delta * power *
//            dotNormalTowardsLightNormal * mat->GetDiffuse(); ret +=
//            mat->GetDiffuseColor(r.obj, p, m_Scene->GetTextures()) *
//            lightMat->GetEmission() * illum;
//        }
//    }
//
//    return ret;
//}

void LightSpot::Intersect(core::Ray &r) const
{
	const float a = dot(r.direction, r.direction);
	const vec3 rPos = r.origin - centroid;

	const float b = dot(r.direction * 2.f, rPos);
	const float rPos2 = dot(rPos, rPos);
	const float c = rPos2 - m_RadiusSquared;

	const float discriminant = (b * b) - (4 * a * c);
	if (discriminant < 0.0f)
		return; // not valid

	const float div2a = 1.f / (2.f * a);
	const float sqrtDis = sqrtf(discriminant);

	const float t1 = ((-b) + sqrtDis) * div2a;
	const float t2 = ((-b) - sqrtDis) * div2a;

	const float tSlow = t1 > 0.0f && t1 < t2 ? t1 : t2; // Take the largest one, one should be positive...
	if (tSlow <= 0.0f || r.t < tSlow)
		return;

	// Check if we hit the spot in the right direction
	const vec3 towardsLight = this->GetPosition() - r.origin;
	const vec3 towardsLightNorm = normalize(towardsLight);
	float cosA = dot(-towardsLightNorm, Direction);
	if (cosA < cosAngleOuter) // not in the light
		return;

	if (cosA < cosAngleInner)
	{
		const float del = (cosA - cosAngleOuter) / (cosAngleInner - cosAngleOuter);
		const float delta = (del * del) * (del * del);
		return;
	}

	r.t = tSlow;
	r.obj = this;
}

bvh::AABB LightSpot::GetBounds() const
{
	const float radius = sqrtf(m_RadiusSquared);
	return bvh::AABB(vec3(centroid - radius) - EPSILON, vec3(centroid + radius) + EPSILON);
}

vec3 LightSpot::GetRandomPointOnSurface(const vec3 &direction, vec3 &lNormal, RandomGenerator &rng) const
{
	const vec3 point = rng.RandomPointOnHemisphere(direction);
	lNormal = point;
	return centroid + point * sqrtf(this->m_RadiusSquared);
}

glm::vec3 LightSpot::GetNormal(const glm::vec3 &hitPoint) const { return normalize(hitPoint - centroid); }

glm::vec2 LightSpot::GetTexCoords(const glm::vec3 &hitPoint) const { return glm::vec2(); }
} // namespace prims