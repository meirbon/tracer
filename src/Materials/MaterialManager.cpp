#include "MaterialManager.h"

using namespace core;

static MaterialManager *instance = nullptr;

MaterialManager *MaterialManager::GetInstance()
{
	if (!instance)
	{
		instance = new MaterialManager();
	}

	return instance;
}

Material &MaterialManager::GetMaterial(size_t index) { return m_Materials.at(index); }

Microfacet &MaterialManager::GetMicrofacet(size_t index) { return m_Microfacets.at(index); }

Surface &MaterialManager::GetTexture(size_t index) { return *m_Textures.at(index); }

const Material &MaterialManager::GetMaterial(size_t index) const { return m_Materials.at(index); }

const Microfacet &MaterialManager::GetMicrofacet(size_t index) const { return m_Microfacets.at(index); }

const Surface &MaterialManager::GetTexture(size_t index) const { return *m_Textures.at(index); }

size_t MaterialManager::AddMaterial(Material mat)
{
	auto idx = m_Materials.size();
	m_Materials.push_back(mat);
	m_Microfacets.emplace_back(mat.diffuse, mat.diffuse);
	return idx;
}

size_t MaterialManager::AddMicrofacet(Microfacet mat)
{
	auto idx = m_Microfacets.size();
	m_Microfacets.push_back(mat);
	return idx;
}

size_t MaterialManager::AddTexture(const char *path)
{
	auto idx = m_Textures.size();
	m_Textures.push_back(new Surface(path));
	return idx;
}

void MaterialManager::Delete()
{
	auto *pointer = instance;
	instance = nullptr;
	delete pointer;
}

MaterialManager::MaterialManager() { AddMaterial(Material::lambertian(glm::vec3(1.0f))); }
