#include "GpuTriangleList.h"

void GpuTriangleList::AddTriangle(GpuTriangle triangle)
{
    const auto idx = static_cast<unsigned int>(m_Triangles.size());
    triangle.objIdx = idx;
    m_Triangles.push_back(triangle);
    m_Aabbs.push_back(triangle.GetBounds());
    m_PrimIndices.push_back(idx);
}

void GpuTriangleList::AddLight(GpuTriangle triangle)
{
    const auto idx = static_cast<unsigned int>(m_Triangles.size());
    triangle.objIdx = idx;
    m_Triangles.push_back(triangle);
    m_LightIndices.push_back(idx);
    m_Aabbs.push_back(triangle.GetBounds());
    m_PrimIndices.push_back(idx);
}

void GpuTriangleList::TraceRay(Ray &r) const {}

const std::vector<SceneObject *> &GpuTriangleList::GetLights() const
{
    throw "This should never be called";
}

void GpuTriangleList::ConstructBVH() {}

unsigned int GpuTriangleList::GetPrimitiveCount()
{
    return static_cast<unsigned int>(m_PrimIndices.size());
}

bool GpuTriangleList::TraceShadowRay(Ray &r, float tMax) const { return true; }

unsigned int GpuTriangleList::TraceDebug(Ray &r) const { return 0; }

GpuTriangle::GpuTriangle(vec3 p0, vec3 p1, vec3 p2, uint matIndex)
    : p0(p0), p1(p1), p2(p2), matIdx(matIndex)
{
    this->normal = normalize(cross(p1 - p0, p2 - p0));
    this->n0 = this->n1 = this->n2 = normal;
    m_Area = CalcArea();
}

GpuTriangle::GpuTriangle(vec3 p0, vec3 p1, vec3 p2, uint matIndex, vec2 t0,
                         vec2 t1, vec2 t2)
    : p0(p0), p1(p1), p2(p2), matIdx(matIndex), t0(t0), t1(t1), t2(t2)
{
    this->normal = normalize(cross(p1 - p0, p2 - p0));
    this->n0 = this->n1 = this->n2 = normal;
    m_Area = CalcArea();
}

GpuTriangle::GpuTriangle(vec3 p0, vec3 p1, vec3 p2, vec3 n0, vec3 n1, vec3 n2,
                         uint matIndex)
    : p0(p0), p1(p1), p2(p2), n0(n0), n1(n1), n2(n2), matIdx(matIndex)
{
    this->normal = normalize(cross(p1 - p0, p2 - p0));
    m_Area = CalcArea();
}

GpuTriangle::GpuTriangle(vec3 p0, vec3 p1, vec3 p2, vec3 n0, vec3 n1, vec3 n2,
                         uint matIndex, vec2 t0, vec2 t1, vec2 t2)
    : p0(p0), p1(p1), p2(p2), n0(n0), n1(n1), n2(n2), matIdx(matIndex), t0(t0),
      t1(t1), t2(t2)
{
    this->normal = normalize(cross(p1 - p0, p2 - p0));
    m_Area = CalcArea();
}

vec3 GpuTriangle::GetCentroid() const { return (p0 + p1 + p2) / 3.f; }

bvh::AABB GpuTriangle::GetBounds() const
{
    const float minX = glm::min(glm::min(p0.x, p1.x), p2.x);
    const float minY = glm::min(glm::min(p0.y, p1.y), p2.y);
    const float minZ = glm::min(glm::min(p0.z, p1.z), p2.z);

    const float maxX = glm::max(glm::max(p0.x, p1.x), p2.x);
    const float maxY = glm::max(glm::max(p0.y, p1.y), p2.y);
    const float maxZ = glm::max(glm::max(p0.z, p1.z), p2.z);
    return {vec3(minX, minY, minZ) - EPSILON, vec3(maxX, maxY, maxZ) + EPSILON};
}

float GpuTriangle::CalcArea() const
{
    // Heron's formula
    const float a = glm::length(p0 - p1);
    const float b = glm::length(p1 - p2);
    const float c = glm::length(p2 - p0);
    const float s = (a + b + c) / 2.f;
    return sqrtf(s * (s - a) * (s - b) * (s - c));
}