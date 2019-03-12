#include "Torus.h"
#include "Core/Ray.h"
#include "Shared.h"
#include "Utils/QuarticSolver.h"

using namespace glm;

#define EQN_EPS 1e-5

Torus::Torus(glm::vec3 center, glm::vec3 vAxis, glm::vec3 hAxis,
             float innerRadius, float outerRadius, unsigned int matIdx)
{
    this->m_Axis = normalize(vAxis);
    this->m_HorizontalAxis = normalize(cross(normalize(hAxis), m_Axis));
    this->m_InnerRadius = innerRadius;
    this->m_OuterRadius = outerRadius;
    this->materialIdx = matIdx;
    this->m_TorusPlaneOffset = dot(m_Axis, center);
    this->centroid = center;
}

void Torus::Intersect(Ray &r) const
{
    const double innerRadius2 = m_InnerRadius * m_InnerRadius;
    const double outerRadius2 = m_OuterRadius * m_OuterRadius;

    const vec3 centerToRayOrigin = r.origin - centroid;
    const double centerToRayOriginDotDirection =
        dot(r.direction, centerToRayOrigin);
    const double centerToRayOriginDotDirectionSquared =
        dot(centerToRayOrigin, centerToRayOrigin);
    const double axisDotCenterToRayOrigin = dot(m_Axis, centerToRayOrigin);
    const double axisDotRayDirection = dot(m_Axis, r.direction);
    const double a = 1 - axisDotRayDirection * axisDotRayDirection;
    const double b = 2 * (dot(centerToRayOrigin, r.direction) -
                          axisDotCenterToRayOrigin * axisDotRayDirection);
    const double c = centerToRayOriginDotDirectionSquared -
                     axisDotCenterToRayOrigin * axisDotCenterToRayOrigin;
    const double d =
        centerToRayOriginDotDirectionSquared + outerRadius2 - innerRadius2;

    const double A = 1;
    const double B = 4 * centerToRayOriginDotDirection;
    const double C = 2 * d + B * B * 0.25 - 4 * outerRadius2 * a;
    const double D = B * d - 4 * outerRadius2 * b;
    const double E = d * d - 4 * outerRadius2 * c;

    QuarticEquation equation(A, B, C, D, E);
    double roots[4] = {-1.f, -1.f, -1.f, -1.f}; // solutions
    const int rootsCount = equation.Solve(roots);

    if (rootsCount == 0)
        return;

    // Find closest to zero positive solution
    bool foundRoot = false;
    float closestRoot = FLT_MAX;
    for (auto &root : roots)
    {
        if (root > EQN_EPS && root < closestRoot)
        {
            closestRoot = float(root);
            foundRoot = true;
        }
    }

    if (foundRoot && closestRoot < r.t)
    {
        r.t = closestRoot;
        r.obj = this;
    }
}

bvh::AABB Torus::GetBounds() const
{
    return {centroid +
                vec3(-1.f, -1.f, -1.f) * (m_OuterRadius + 2 * m_InnerRadius) -
                EPSILON,
            centroid +
                vec3(1.f, 1.f, 1.f) * (m_OuterRadius + 2 * m_InnerRadius) +
                EPSILON};
}

vec3 Torus::GetRandomPointOnSurface(const vec3 &direction, vec3 &lNormal,
                                    RandomGenerator &rng) const
{
    return glm::vec3(0.f);
}

glm::vec3 Torus::GetNormal(const glm::vec3 &hitPoint) const
{
    const double offsetHitPlane = dot(m_Axis, hitPoint);
    const auto offsetDiff = float(m_TorusPlaneOffset - offsetHitPlane);
    const vec3 projectedHitpoint = hitPoint + (m_Axis * offsetDiff);
    const vec3 centerToProjectedHit =
        normalize(projectedHitpoint - centroid) * m_OuterRadius;
    const vec3 projectedHit = centroid + centerToProjectedHit;
    return normalize(hitPoint - projectedHit);
}

glm::vec2 Torus::GetTexCoords(const glm::vec3 &hitPoint) const
{
    const double offsetHitPlane = dot(m_Axis, hitPoint);
    const auto offsetDiff = float(m_TorusPlaneOffset - offsetHitPlane);
    const vec3 projectedHitpoint = hitPoint + (m_Axis * offsetDiff);
    const vec3 centerToProjectedHitNormalized =
        normalize(projectedHitpoint - centroid);
    const vec3 centerToProjectedHit =
        centerToProjectedHitNormalized * m_OuterRadius;
    const vec3 projectedHit = centroid + centerToProjectedHit;
    const vec3 normal = normalize(hitPoint - projectedHit);

    float u = dot(m_HorizontalAxis, centerToProjectedHitNormalized);
    float v = dot(normal, m_Axis);

    if (u < 0)
        u += 1.f;
    if (v < 0)
        v += 1.f;

    return vec2(u, v);
}