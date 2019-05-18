#include "LightDirectional.h"
#include "BVH/TopLevelBVH.h"
#include "Core/Ray.h"
#include "Materials/MaterialManager.h"
#include "Shared.h"

using namespace glm;

namespace prims
{

LightDirectional::LightDirectional(vec3 direction, unsigned int matIdx)
{
	this->Direction = normalize(direction);
	this->materialIdx = matIdx;
	this->centroid = -1e33f * direction;
}

// glm::vec3 LightDirectional::CalculateLight(
//    const Ray& r,
//    const material::Material* mat,
//    const WorldScene* m_Scene) const
//{
//    glm::vec3 ret = glm::vec3(0.f);
//    const vec3 p = r.GetHitpoint();
//    auto normal = r.obj->GetNormal(p);
//
//    if (dot(r.direction, normal) > 0) {
//        normal = -normal;
//    }
//
//    if (dot(normal, -Direction) <= 0) { // object facing away from light
//        return ret;
//    }
//
//    Ray shadowRay = Ray(p + EPSILON * -Direction, -Direction);
//    m_Scene->TraceRay(shadowRay);
//
//    if (!shadowRay.IsValid() || shadowRay.obj == this) {
//        const auto lightMat =
//        material::MaterialManager::GetInstance()->GetMaterial(this->materialIdx);
//        const vec3 reflectDir = (r.direction - (2.0f * dot(r.direction,
//        normal) * normal));
//
//        const float illum = lightMat.GetIntensity();
//
//        if (mat->IsReflective()) {
//            const float specular = dot(-Direction, normalize(reflectDir)) *
//            illum; const glm::vec3 specularColor = lightMat.GetEmission() *
//            illum * powf(specular, mat->shininess); ret +=
//            mat->GetSpecularColor() * specularColor * mat->GetSpecular();
//        }
//
//        if (mat->IsDiffuse()) {
//            const glm::vec3 lightColor = lightMat.GetEmission() * illum *
//            glm::max(dot(-Direction, normal), 0.f); ret +=
//            mat->GetDiffuseColor(r.obj, p) * lightColor * mat->GetDiffuse();
//        }
//    }
//
//    return ret;
//}

void LightDirectional::Intersect(core::Ray &r) const
{
#if !PATH_TRACER
	return;
#endif

	if (dot(r.direction, Direction) > 0)
	{
		return;
	}

	r.t = 1e33f;
	r.obj = this;
}

bvh::AABB LightDirectional::GetBounds() const { return bvh::AABB(vec3(0.f), vec3(0.f)); }

vec3 LightDirectional::GetRandomPointOnSurface(const vec3 &direction, vec3 &lNormal, RandomGenerator &rng) const
{
	vec3 other;
	if (direction.z == 0 && direction.y == 0)
	{
		other = {direction.y, direction.x, direction.z};
	}
	else
	{
		other = {direction.x, direction.z, direction.y};
	}
	const glm::vec3 right = normalize(cross(direction, other));
	const glm::vec3 up = normalize(cross(direction, right));

	lNormal = this->Direction;
	return this->Direction * -1e33f + (rng.Rand(2.f) - 1.f) * 1e33f * up + (rng.Rand(2.f) - 1.f) * 1e33f * right;
}

glm::vec3 LightDirectional::GetNormal(const glm::vec3 &) const { return Direction; }

glm::vec2 LightDirectional::GetTexCoords(const glm::vec3 &) const { return glm::vec2(); }
} // namespace prims