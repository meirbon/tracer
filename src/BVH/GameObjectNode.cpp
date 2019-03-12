#include <glm/ext.hpp>

#include "GameObject.h"
#include "GameObjectNode.h"

using namespace glm;

namespace bvh
{
const __m128 QuadOne = _mm_set1_ps(1.f);

bool GameObjectNode::Intersect(const Ray &r) const
{
    const __m128 dirInversed = _mm_div_ps(QuadOne, r.m_Direction4);
    const __m128 t1 = _mm_mul_ps(
        _mm_sub_ps(boundsWorldSpace.bmin4, r.m_Origin4), dirInversed);
    const __m128 t2 = _mm_mul_ps(
        _mm_sub_ps(boundsWorldSpace.bmax4, r.m_Origin4), dirInversed);

    union {
        __m128 f4;
        float f[4];
    } qvmax, qvmin;

    qvmax.f4 = _mm_max_ps(t1, t2);
    qvmin.f4 = _mm_min_ps(t1, t2);

    const float tmax = glm::min(qvmax.f[0], glm::min(qvmax.f[1], qvmax.f[2]));
    const float tmin = glm::max(qvmin.f[0], glm::max(qvmin.f[1], qvmin.f[2]));

    return tmax >= 0 && tmin < tmax;
}

bool GameObjectNode::Intersect(const Ray &r, float &tnear, float &tfar) const
{
    const auto newOrigin = transformationMat * vec4(r.origin, 1.0f);
    const auto newDirection = transformationMat * vec4(r.direction, 0.0f);

    const auto org4 = _mm_load_ps(glm::value_ptr(newOrigin));
    const auto dir4 = _mm_load_ps(glm::value_ptr(newDirection));

    const __m128 dirInversed = _mm_div_ps(QuadOne, dir4);
    const __m128 t1 =
        _mm_mul_ps(_mm_sub_ps(boundsWorldSpace.bmin4, org4), dirInversed);
    const __m128 t2 =
        _mm_mul_ps(_mm_sub_ps(boundsWorldSpace.bmax4, org4), dirInversed);

    union {
        __m128 f4;
        float f[4];
    } qvmax, qvmin;

    qvmax.f4 = _mm_max_ps(t1, t2);
    qvmin.f4 = _mm_min_ps(t1, t2);

    tfar = glm::min(qvmax.f[0], glm::min(qvmax.f[1], qvmax.f[2]));
    tnear = glm::max(qvmin.f[0], glm::max(qvmin.f[1], qvmin.f[2]));

    return tfar >= 0 && tnear < tfar;
}

void GameObjectNode::Traverse(Ray &rOrg) const
{
    const vec4 org = transformationMat * vec4(rOrg.origin, 1.f);
    const vec4 dir = transformationMat * vec4(rOrg.direction, 0.f);
    Ray r = {{org.x, org.y, org.z}, {dir.x, dir.y, dir.z}};
    r.t = rOrg.t;
    this->gameObject->m_BVHTree->TraceRay(r);

    if (r.IsValid())
    {
        rOrg.t = r.t;
        rOrg.obj = r.obj;
        const vec4 normal =
            inverseMat * vec4(r.obj->GetNormal(r.GetHitpoint()), 0.f);
        rOrg.normal = normalize(vec3(normal.x, normal.y, normal.z));
    }
}

unsigned int GameObjectNode::TraverseDebug(Ray &rOrg) const
{
    unsigned int depth = 0;
    const vec4 org = transformationMat * vec4(rOrg.origin, 1.f);
    const vec4 dir = transformationMat * vec4(rOrg.direction, 0.f);
    Ray r = {{org.x, org.y, org.z}, {dir.x, dir.y, dir.z}};
    r.t = rOrg.t;

    return gameObject->m_BVHTree->TraceDebug(r);
}

void GameObjectNode::TraverseShadow(Ray &rOrg) const
{
    const vec4 org = transformationMat * vec4(rOrg.origin, 1.f);
    const vec4 dir = transformationMat * vec4(rOrg.direction, 0.f);
    Ray r = {{org.x, org.y, org.z}, {dir.x, dir.y, dir.z}};
    r.t = rOrg.t;
    this->gameObject->m_BVHTree->TraceRay(r);

    if (r.IsValid())
        rOrg.t = r.t;
}

GameObjectNode::GameObjectNode(glm::mat4 transformationMat,
                               glm::mat4 inverseMat,
                               bvh::GameObject *gameObject)
{
    this->transformationMat = transformationMat;
    this->inverseMat = inverseMat;
    this->gameObject = gameObject;
    this->ConstructWorldBounds();
}

void GameObjectNode::ConstructWorldBounds()
{
    const AABB bounds = gameObject->GetBounds();

    vec4 p1 = vec4(bounds.bmin[0], bounds.bmin[1], bounds.bmin[2], 1.f);
    vec4 p5 = vec4(bounds.bmax[0], bounds.bmax[1], bounds.bmax[2], 1.f);
    vec4 p2 = vec4(p1.x, p1.y, p5.z, 1.f);
    vec4 p3 = vec4(p1.x, p5.y, p1.z, 1.f);
    vec4 p4 = vec4(p5.x, p1.y, p1.z, 1.f);
    vec4 p6 = vec4(p5.x, p5.y, p1.z, 1.f);
    vec4 p7 = vec4(p5.x, p1.y, p5.z, 1.f);
    vec4 p8 = vec4(p1.x, p5.y, p5.z, 1.f);

    p1 = this->inverseMat * p1;
    p2 = this->inverseMat * p2;
    p3 = this->inverseMat * p3;
    p4 = this->inverseMat * p4;
    p5 = this->inverseMat * p5;
    p6 = this->inverseMat * p6;
    p7 = this->inverseMat * p7;
    p8 = this->inverseMat * p8;

    const float minX = glm::min(
        p1.x,
        glm::min(p2.x,
                 glm::min(p3.x,
                          glm::min(p4.x,
                                   glm::min(p5.x,
                                            glm::min(p6.x,
                                                     glm::min(p7.x, p8.x)))))));
    const float minY = glm::min(
        p1.y,
        glm::min(p2.y,
                 glm::min(p3.y,
                          glm::min(p4.y,
                                   glm::min(p5.y,
                                            glm::min(p6.y,
                                                     glm::min(p7.y, p8.y)))))));
    const float minZ = glm::min(
        p1.z,
        glm::min(p2.z,
                 glm::min(p3.z,
                          glm::min(p4.z,
                                   glm::min(p5.z,
                                            glm::min(p6.z,
                                                     glm::min(p7.z, p8.z)))))));

    const float maxX = glm::max(
        p1.x,
        glm::max(p2.x,
                 glm::max(p3.x,
                          glm::max(p4.x,
                                   glm::max(p5.x,
                                            glm::max(p6.x,
                                                     glm::max(p7.x, p8.x)))))));
    const float maxY = glm::max(
        p1.y,
        glm::max(p2.y,
                 glm::max(p3.y,
                          glm::max(p4.y,
                                   glm::max(p5.y,
                                            glm::max(p6.y,
                                                     glm::max(p7.y, p8.y)))))));
    const float maxZ = glm::max(
        p1.z,
        glm::max(p2.z,
                 glm::max(p3.z,
                          glm::max(p4.z,
                                   glm::max(p5.z,
                                            glm::max(p6.z,
                                                     glm::max(p7.z, p8.z)))))));

    const vec3 min = vec3(minX, minY, minZ);
    const vec3 max = vec3(maxX, maxY, maxZ);

    this->boundsWorldSpace = {min - EPSILON, max + EPSILON};
}
} // namespace bvh