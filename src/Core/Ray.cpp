#include "Ray.h"
#include "Shared.h"
#include "Utils/Template.h"

using namespace glm;

Ray::Ray(const vec3 &a, const vec3 &b)
{
    this->origin = a;
    this->direction = b;
    this->t = 1e34f;
    this->obj = nullptr;
}

vec3 Ray::GetHitpoint() const { return origin + t * direction; }

Ray Ray::Reflect(const vec3 &point, const vec3 &normal) const
{
    const vec3 reflectDir =
        (direction - (2.f * dot(normal, direction) * normal));
    return {point + EPSILON * reflectDir, reflectDir};
}

Ray Ray::DiffuseReflection(const vec3 &point, const vec3 &normal,
                           RandomGenerator &rng) const
{
    const vec3 dir = RandomPointOnHemisphere(normal, rng);
    return {point + EPSILON * dir, dir};
}

Ray Ray::Reflect(const vec3 &normal) const
{
    const vec3 point = GetHitpoint();
    return Reflect(point, normal);
}

Ray Ray::DiffuseReflection(const vec3 &normal, RandomGenerator &rng) const
{
    const vec3 point = GetHitpoint();
    return DiffuseReflection(point, normal, rng);
}

// http://www.rorydriscoll.com/2009/01/07/better-sampling/
vec3 cosineWeightedSample(const float &r1, const float &r2)
{
    const float r = sqrtf(r1);
    float theta = 2.0f * PI * r2;
    float x = r * cosf(theta);
    float z = r * sinf(theta);
    return vec3(x, fmax(0.0f, sqrtf(1.0f - r1)), z);
}

Ray Ray::CosineWeightedDiffuseReflection(const vec3 &origin, const vec3 &normal,
                                         RandomGenerator &rng) const
{
    vec3 Nt, Nb;
    createCoordinateSystem(normal, Nt, Nb);
    const float r1 = rng.Rand(1.0f);
    const float r2 = rng.Rand(1.0f);
    const vec3 sample = cosineWeightedSample(r1, r2);
    const vec3 dir = localToWorld(sample, Nt, Nb, normal);
    return {origin + EPSILON * dir, dir};
}

glm::vec3 Ray::TransformToTangent(const vec3 &normal, vec3 vector) const
{
    const vec3 w = abs(normal.x > 0.99f) ? vec3(0, 1, 0) : vec3(1, 0, 0);
    const vec3 t = normalize(cross(normal, w));
    const vec3 b = cross(t, normal);
    return (vector * t, vector * b, vector * normal);
}