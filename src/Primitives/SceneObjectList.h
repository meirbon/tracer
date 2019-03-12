#pragma once

#include <vector>

#include "BVH/AABB.h"
#include "Core/Ray.h"
#include "Materials/Material.h"
#include "Materials/MaterialMicrofacet.h"
#include "Materials/Microfacet.h"
#include "Primitives/Light.h"
#include "Primitives/LightDirectional.h"
#include "Primitives/LightPoint.h"
#include "Primitives/LightSpot.h"
#include "Primitives/SceneObject.h"
#include "Renderer/Surface.h"

class WorldScene
{
  public:
    virtual void TraceRay(Ray &r) const = 0;

    virtual bool TraceShadowRay(Ray &r, float tMax) const = 0;

    virtual const std::vector<SceneObject *> &GetLights() const = 0;

    virtual void ConstructBVH() {}

    virtual bvh::AABB GetNodeBounds(unsigned int index) = 0;

    virtual unsigned int GetPrimitiveCount() = 0;

    virtual ~WorldScene() = default;

    virtual unsigned int TraceDebug(Ray &r) const = 0;
};

class SceneObjectList : public WorldScene
{
  public:
    SceneObjectList();

    ~SceneObjectList() override;

    void AddObject(SceneObject *object);

    void AddLight(SceneObject *light);

    const std::vector<SceneObject *> &GetLights() const override;

    void TraceRay(Ray &r) const override;

    bool TraceShadowRay(Ray &r, float tMax) const override;

    const std::vector<SceneObject *> &GetObjects() const;

    const std::vector<bvh::AABB> &GetAABBs() const;

    std::vector<unsigned int> GetPrimitiveIndices() { return m_PrimIndices; }

    inline unsigned int GetPrimitiveCount() override
    {
        return static_cast<unsigned int>(m_List.size());
    }

    inline bvh::AABB GetNodeBounds(unsigned int index) override
    {
        return m_List[index]->GetBounds();
    }

    inline unsigned int TraceDebug(Ray &r) const override
    {
        unsigned int depth = 0;
        for (auto obj : m_List)
        {
            depth++;
            obj->Intersect(r);
        }
        return depth;
    }

  private:
    std::vector<SceneObject *> m_List;
    std::vector<SceneObject *> m_Lights;
    std::vector<bvh::AABB> m_Aabbs;
    std::vector<unsigned int> m_PrimIndices;
};
