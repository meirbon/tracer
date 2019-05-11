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
	const std::string p = path;
	if (m_LoadedTextures.find(p) != m_LoadedTextures.end()) // Texture already in memory
		return m_LoadedTextures.at(path);

	auto *tex = new core::Surface(path);
	size_t idx = m_Textures.size();
	m_LoadedTextures[path] = idx;
	m_Textures.push_back(tex);
	return idx;
}

void MaterialManager::Delete()
{
	auto *pointer = instance;
	instance = nullptr;
	delete pointer;
}

MaterialManager::MaterialManager() { AddMaterial(Material::lambertian(glm::vec3(1.0f))); }

MaterialManager::GPUTextures MaterialManager::createTextureBuffer() const
{
	using namespace glm;

	std::vector<unsigned int> tDims;
	std::vector<unsigned int> tOffsets;
	std::vector<vec4> tColors;

	for (auto &tex : m_Textures)
	{
		unsigned int offset = tColors.size();
		const vec4 *buffer = tex->GetTextureBuffer();
		const uint width = tex->getWidth();
		const uint height = tex->getHeight();
		for (uint y = 0; y < height; y++)
		{
			for (uint x = 0; x < width; x++)
			{
				tColors.push_back(buffer[x + y * width]);
			}
		}

		tOffsets.push_back(offset);
		tDims.push_back(width);
		tDims.push_back(height);
	}

	GPUTextures buffer;
	buffer.textureDims = std::move(tDims);
	buffer.textureOffsets = std::move(tOffsets);
	buffer.textureColors = std::move(tColors);
	return buffer;
}
