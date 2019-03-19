#include "Primitives/SceneObjectList.h"

namespace prims
{
SceneObjectList::SceneObjectList()
{
	m_List = std::vector<SceneObject *>();
	m_Lights = std::vector<SceneObject *>();
}

SceneObjectList::~SceneObjectList()
{
	for (auto *object : m_List)
		delete object;
}

void SceneObjectList::AddObject(SceneObject *object)
{
	const auto idx = m_List.size();
	object->objIdx = static_cast<unsigned int>(idx);
	m_List.push_back(object);
	m_Aabbs.push_back(object->GetBounds());
	m_PrimIndices.push_back(idx);
}

void SceneObjectList::AddLight(SceneObject *light)
{
	const auto idx = m_List.size();
	light->objIdx = static_cast<unsigned int>(idx);
	m_PrimIndices.push_back(idx);
	if (dynamic_cast<LightDirectional *>(light) == nullptr)
	{
		m_List.push_back(light);
	}
	m_Lights.push_back(light);
	m_Aabbs.push_back(light->GetBounds());
}

const std::vector<SceneObject *> &SceneObjectList::GetLights() const { return m_Lights; }

void SceneObjectList::TraceRay(core::Ray &r) const
{
	for (SceneObject *object : m_List)
	{
		object->Intersect(r);
	}

	r.normal = r.obj->GetNormal(r.GetHitpoint());
}

const std::vector<SceneObject *> &SceneObjectList::GetObjects() const { return m_List; }

const std::vector<bvh::AABB> &SceneObjectList::GetAABBs() const { return m_Aabbs; }

bool SceneObjectList::TraceShadowRay(core::Ray &r, float tMax) const
{
	for (SceneObject *object : m_List)
	{
		object->Intersect(r);
	}

	return r.t < tMax;
}
} // namespace prims