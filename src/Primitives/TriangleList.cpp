#include "Primitives/TriangleList.h"
#include "Materials/MaterialManager.h"
#include "Primitives/SOATriangle.h"

#include <iostream>

using namespace glm;

static MaterialManager *mInstance = MaterialManager::GetInstance();

namespace prims
{

void TriangleList::addTriangle(vec3 p0, vec3 p1, vec3 p2, vec3 n0, vec3 n1, vec3 n2, unsigned int matIdx, vec2 t0,
							   vec2 t1, vec2 t2)
{
	const auto idx0 = m_Vertices.size();
	const auto idx1 = idx0 + 1;
	const auto idx2 = idx0 + 2;

	m_Vertices.emplace_back(p0.x, p0.y, p0.z, 0.0f);
	m_Vertices.emplace_back(p1.x, p1.y, p1.z, 0.0f);
	m_Vertices.emplace_back(p2.x, p2.y, p2.z, 0.0f);

	const vec3 n0n = normalize(n0);
	const vec3 n1n = normalize(n1);
	const vec3 n2n = normalize(n2);

	m_Normals.emplace_back(n0n.x, n0n.y, n0n.z, 0.0f);
	m_Normals.emplace_back(n1n.x, n1n.y, n1n.z, 0.0f);
	m_Normals.emplace_back(n2n.x, n2n.y, n2n.z, 0.0f);

	const vec3 cn = normalize((n0 + n1 + n2) / 3.0f);
	m_CenterNormals.emplace_back(cn.x, cn.y, cn.z, 0.0f);

	m_TexCoords.push_back(t0);
	m_TexCoords.push_back(t1);
	m_TexCoords.push_back(t2);

	m_Indices.emplace_back(idx0, idx1, idx2, 0);

	m_MaterialIdxs.push_back(matIdx);

	m_AABBs.push_back(triangle::getBounds(p0, p1, p2));

	if (mInstance->GetMaterial(matIdx).type == Light)
		m_LightIndices.push_back(m_Indices.size() - 1);
}

void TriangleList::loadModel(const std::string &path, float scale, mat4 mat, int material, bool norm)
{
	Assimp::Importer importer;
	importer.SetPropertyBool(AI_CONFIG_PP_PTV_NORMALIZE, norm);
	const aiScene *scene = importer.ReadFile(
		path, aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality |
				  aiProcess_LimitBoneWeights | aiProcess_FlipWindingOrder | aiProcess_RemoveRedundantMaterials |
				  aiProcess_Triangulate | aiProcess_FindInvalidData | aiProcess_PreTransformVertices);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "Error Assimp: " << importer.GetErrorString() << std::endl;
		return;
	}

	std::vector<TriangleList::Mesh> meshes = {};

	std::string directory = path.substr(0, path.find_last_of('/'));
	processNode(scene->mRootNode, scene, meshes, directory);

	unsigned int offset = m_Vertices.size();

	for (const auto &mesh : meshes)
	{
		for (size_t i = 0; i < mesh.positions.size(); i++)
		{
			vec4 pos = mat * vec4(mesh.positions[i] * scale, 1.0f);
			vec4 normal = mat * vec4(mesh.normals[i], 0.0f);

			pos.w = 0.0f;
			normal.w = 0.0f;

			m_Vertices.push_back(pos);
			m_Normals.push_back(normal);
			m_TexCoords.push_back(mesh.texCoords[i]);
		}
	}

	for (const auto &mesh : meshes)
	{
		for (size_t i = 0; i < mesh.indices.size(); i += 3)
		{
			const auto idx0 = mesh.indices[i + 0];
			const auto idx1 = mesh.indices[i + 1];
			const auto idx2 = mesh.indices[i + 2];

			const auto realIdx0 = idx0 + offset;
			const auto realIdx1 = idx1 + offset;
			const auto realIdx2 = idx2 + offset;

			m_Indices.emplace_back(realIdx0, realIdx1, realIdx2, 0);
			unsigned int matIdx;

			if (material < 0)
				matIdx = mesh.materialIdx;
			else
				matIdx = material;

			if (mInstance->GetMaterial(matIdx).type == Light)
				m_LightIndices.push_back(m_Indices.size() - 1);

			m_MaterialIdxs.push_back(matIdx);

			const vec3 vec0 = vec3(m_Vertices[realIdx0].x, m_Vertices[realIdx0].y, m_Vertices[realIdx0].z);
			const vec3 vec1 = vec3(m_Vertices[realIdx1].x, m_Vertices[realIdx1].y, m_Vertices[realIdx1].z);
			const vec3 vec2 = vec3(m_Vertices[realIdx2].x, m_Vertices[realIdx2].y, m_Vertices[realIdx2].z);
			const vec3 normal = -normalize(cross(vec1 - vec0, vec2 - vec0));
			m_CenterNormals.emplace_back(normal, 0.0);

			m_AABBs.push_back(triangle::getBounds(m_Vertices[realIdx0], m_Vertices[realIdx1], m_Vertices[realIdx2]));
		}

		offset += mesh.positions.size();
	}
}

void TriangleList::processNode(aiNode *node, const aiScene *scene, std::vector<Mesh> &meshes, const std::string &dir)
{
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(processMesh(mesh, scene, dir));
	}

	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		processNode(node->mChildren[i], scene, meshes, dir);
	}
}

TriangleList::Mesh TriangleList::processMesh(aiMesh *mesh, const aiScene *scene, const std::string &dir)
{
	Mesh m{};

	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		m.positions.emplace_back(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
		m.normals.push_back(glm::normalize(vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z)));
		if (mesh->mTextureCoords[0])
		{
			float x = mesh->mTextureCoords[0][i].x;
			float y = mesh->mTextureCoords[0][i].y;

			m.texCoords.emplace_back(x, y);
		}
		else
			m.texCoords.emplace_back(0.5f, 0.5f);
	}

	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace &face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
		{
			m.indices.emplace_back(face.mIndices[j]);
		}
	}

	aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

	// return a mesh object created from the extracted mesh data

	auto isNotZero = [](aiColor3D col) { return col.r > 0.0001f && col.g > 0.0001f && col.b > 0.0001f; };

	Material mat;
	aiColor3D color;
	int dTexIdx = -1, nTexIdx = -1, mTexIdx = -1, dispTexIdx = -1;

	for (uint i = 0; i < material->GetTextureCount(aiTextureType_DIFFUSE); i++)
	{
		aiString str;
		material->GetTexture(aiTextureType_DIFFUSE, i, &str);
		std::string path = dir + '/' + str.C_Str();
		dTexIdx = loadTexture(path);
	}

	for (uint i = 0; i < material->GetTextureCount(aiTextureType_NORMALS); i++)
	{
		aiString str;
		material->GetTexture(aiTextureType_NORMALS, i, &str);
		std::string path = dir + '/' + str.C_Str();
		nTexIdx = loadTexture(path);
	}

	for (uint i = 0; i < material->GetTextureCount(aiTextureType_OPACITY); i++)
	{
		aiString str;
		material->GetTexture(aiTextureType_OPACITY, i, &str);
		std::string path = dir + '/' + str.C_Str();
		mTexIdx = loadTexture(path);
	}

	for (uint i = 0; i < material->GetTextureCount(aiTextureType_DISPLACEMENT); i++)
	{
		aiString str;
		material->GetTexture(aiTextureType_DISPLACEMENT, i, &str);
		std::string path = dir + '/' + str.C_Str();
		dispTexIdx = loadTexture(path);
	}

	if (dTexIdx > -1)
	{
		float roughness;
		if (material->Get(AI_MATKEY_SHININESS, roughness) != AI_SUCCESS || roughness < 2.0f)
			mat = Material::lambertian(vec3(1.0f), dTexIdx, nTexIdx, mTexIdx, dispTexIdx);
		else
		{
			mat = Material::lambertian(vec3(1.0f), dTexIdx, nTexIdx, mTexIdx, dispTexIdx);
			//			mat.roughness = roughness;
		}

		m.materialIdx = mInstance->AddMaterial(mat);
		return m;
	}

	if (material->Get(AI_MATKEY_COLOR_EMISSIVE, color) == AI_SUCCESS && isNotZero(color))
	{
		mat = Material::light({color.r, color.g, color.b});
	}
	else if (material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS && isNotZero(color))
	{
		float roughness;
		aiColor3D specColor;
		if (material->Get(AI_MATKEY_COLOR_SPECULAR, specColor) == AI_SUCCESS)
		{
			const vec3 spec = {specColor.r, specColor.g, specColor.b};
			const vec3 diff = {color.r, color.g, color.b};
			if (dot(spec, spec) > dot(diff, diff))
			{
				color.r = spec.r, color.g = spec.g, color.b = spec.b;
			}
		}

		if (material->Get(AI_MATKEY_SHININESS, roughness) != AI_SUCCESS || roughness < 1)
			mat = Material::lambertian({color.r, color.g, color.b}, dTexIdx, nTexIdx, mTexIdx, dispTexIdx);
		else
		{
			mat = Material::lambertian({color.r, color.g, color.b}, dTexIdx, nTexIdx, mTexIdx, dispTexIdx);
			//			mat.roughness = roughness;
		}
	}
	else if (material->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS && isNotZero(color))
	{
		mat = Material::specular({color.r, color.g, color.b}, 1.0f);
	}
	else if (material->Get(AI_MATKEY_COLOR_TRANSPARENT, color) == AI_SUCCESS && isNotZero(color))
	{
		float refractIdx;
		if (material->Get(AI_MATKEY_REFRACTI, refractIdx) != AI_SUCCESS)
			refractIdx = 1.2f;
		aiColor3D absorption;
		if (material->Get(AI_MATKEY_COLOR_TRANSPARENT, absorption) != AI_SUCCESS)
			absorption = {0.0f, 0.0f, 0.0f};
		mat = Material::fresnel({color.r, color.g, color.b}, refractIdx, {absorption.r, absorption.g, absorption.b},
								dTexIdx, nTexIdx);
	}

	m.materialIdx = mInstance->AddMaterial(mat);
	return m;
}

unsigned int TriangleList::loadTexture(const std::string &path)
{
	return MaterialManager::GetInstance()->AddTexture(path.c_str());
}

std::vector<Triangle *> TriangleList::getTriangles() const
{
	std::vector<Triangle *> triangles(m_Indices.size());
	for (uint i = 0; i < m_Indices.size(); i++)
	{
		const auto &idx = m_Indices.at(i);
		auto *t = new Triangle();
		t->p0 = m_Vertices[idx.x];
		t->p1 = m_Vertices[idx.y];
		t->p2 = m_Vertices[idx.z];

		t->normal = m_CenterNormals[i];

		t->n0 = m_Normals[idx.x];
		t->n1 = m_Normals[idx.y];
		t->n2 = m_Normals[idx.z];

		t->t0 = m_TexCoords[idx.x];
		t->t1 = m_TexCoords[idx.y];
		t->t2 = m_TexCoords[idx.z];

		t->centroid = (t->p0 + t->p1 + t->p2) / 3.0f;
		t->m_Area = t->CalculateArea();
		t->materialIdx = m_MaterialIdxs[i];

		triangles[i] = t;
	}

	return triangles;
}
}