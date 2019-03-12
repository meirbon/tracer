#include "LightPoint.h"
#include "Core/Ray.h"
#include "Materials/MaterialManager.h"
#include "Primitives/SceneObjectList.h"
#include "Shared.h"

using namespace glm;

LightPoint::LightPoint(glm::vec3 pos, float radius, unsigned int matIndex)
{
    this->centroid = pos;
    this->m_RadiusSquared = radius * radius;
    this->materialIdx = matIndex;
    this->m_Area = 4 * PI * radius * radius;
}
//
// glm::vec3 LightPoint::CalculateLight(
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
//    if (dot(normal, towardsLightNorm) <= 0) { // object facing away from light
//        return ret;
//    }
//
//    Ray shadowRay = Ray(p, towardsLightNorm);
//    m_Scene->TraceRay(shadowRay);
//
//    if (!shadowRay.IsValid() || shadowRay.obj == this) { // nothing ocluding
//    light
//        const auto lightMat =
//        material::MaterialManager::GetInstance()->GetMaterial(this->materialIdx);
//        const vec3 reflectDir = (r.direction - (2.0f * dot(r.direction,
//        normal) * normal));
//
//        const float illum = lightMat.GetIntensity() / towardsLightDot;
//
//        if (mat->IsReflective()) {
//            const float specular = dot(towardsLightNorm,
//            normalize(reflectDir)) * illum; const glm::vec3 specularColor =
//            lightMat.GetEmission() * illum * powf(specular, mat->shininess);
//            ret += mat->GetSpecularColor() * specularColor *
//            mat->GetSpecular();
//        }
//
//        if (mat->IsDiffuse()) {
//            const glm::vec3 lightColor = lightMat.GetEmission() * illum *
//            glm::max(dot(towardsLightNorm, normal), 0.f); ret +=
//            mat->GetDiffuseColor(r.obj, p) * lightColor * mat->GetDiffuse();
//        }
//    }
//
//    return ret;
//}

void LightPoint::Intersect(Ray &r) const
{
    const float a = dot(r.direction, r.direction);
    const vec3 rPos = r.origin - centroid;

    const float b = dot(r.direction * 2.f, rPos);
    const float rPos2 = dot(rPos, rPos);
    const float c = rPos2 - m_RadiusSquared;

    const float discriminant = (b * b) - (4 * a * c);
    if (discriminant < 0.f)
        return; // not valid

    const float div2a = 1.f / (2.f * a);
    const float sqrtDis = discriminant > 0.f ? sqrtf(discriminant) : 0.f;

    const float t1 = ((-b) + sqrtDis) * div2a;
    const float t2 = ((-b) - sqrtDis) * div2a;

    const float tSlow =
        t1 > 0 && t1 < t2
            ? t1
            : t2; // Take the largest one, one should be positive...
    if (tSlow <= EPSILON || r.t < tSlow)
        return;

    r.t = tSlow;
    r.obj = this;
}

bvh::AABB LightPoint::GetBounds() const
{
    const float radius = sqrtf(m_RadiusSquared);
    return {vec3(centroid - radius) - EPSILON,
            vec3(centroid + radius) + EPSILON};
}

vec3 LightPoint::GetRandomPointOnSurface(const vec3 &direction, vec3 &lNormal,
                                         RandomGenerator &rng) const
{
    const vec3 pointOnLight =
        centroid + RandomPointOnHemisphere(normalize(direction), rng);
    lNormal = normalize(pointOnLight - centroid);
    return centroid + lNormal * sqrtf(this->m_RadiusSquared);
}

glm::vec3 LightPoint::GetNormal(const glm::vec3 &hitPoint) const
{
    return normalize(hitPoint - centroid);
    ;
}

glm::vec2 LightPoint::GetTexCoords(const glm::vec3 &hitPoint) const
{
    return glm::vec2();
}