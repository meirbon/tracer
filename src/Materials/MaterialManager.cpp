#include "MaterialManager.h"

static material::MaterialManager *instance = nullptr;

material::MaterialManager *material::MaterialManager::GetInstance()
{
    if (!instance)
    {
        instance = new MaterialManager();
    }

    return instance;
}

material::Material &material::MaterialManager::GetMaterial(size_t index)
{
    return m_Materials.at(index);
}

Microfacet &material::MaterialManager::GetMicrofacet(size_t index)
{
    return m_Microfacets.at(index);
}

Surface &material::MaterialManager::GetTexture(size_t index)
{
    return *m_Textures.at(index);
}

const material::Material &
material::MaterialManager::GetMaterial(size_t index) const
{
    return m_Materials.at(index);
}

const Microfacet &material::MaterialManager::GetMicrofacet(size_t index) const
{
    return m_Microfacets.at(index);
}

const Surface &material::MaterialManager::GetTexture(size_t index) const
{
    return *m_Textures.at(index);
}

size_t material::MaterialManager::AddMaterial(Material mat)
{
    auto idx = m_Materials.size();
    m_Materials.push_back(mat);
    m_Microfacets.emplace_back(mat.diffuse, mat.diffuse);
    return idx;
}

size_t material::MaterialManager::AddMicrofacet(Microfacet mat)
{
    auto idx = m_Microfacets.size();
    m_Microfacets.push_back(mat);
    return idx;
}

size_t material::MaterialManager::AddTexture(const char *path)
{
    auto idx = m_Textures.size();
    m_Textures.push_back(new Surface(path));
    return idx;
}

void material::MaterialManager::Delete()
{
    auto *pointer = instance;
    instance = nullptr;
    delete pointer;
}