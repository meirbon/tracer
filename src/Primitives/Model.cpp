#include "Primitives/Model.h"
#include "Materials/MaterialManager.h"
#include "Primitives/Triangle.h"
#include "Utils/Messages.h"

#include <iostream>

#define TINYOBJLOADER_IMPLEMENTATION

#include "Utils/tiny_obj_loader.h"

void prims::Load(const std::string &inputFile, uint matIndex, vec3 translation, float scale,
				 SceneObjectList *objectList, glm::mat4 transform)
{
	std::vector<uint> fMaterialsIndices;

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string err;
	std::string directory = inputFile.substr(0, inputFile.find_last_of('/')) + "/";
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, inputFile.c_str(), directory.c_str());
	if (!err.empty())
		std::cout << "Model Error: " << err << std::endl;

	if (!ret)
	{
		utils::FatalError(__FILE__, __LINE__, err.c_str(), "TinyOBJ");
		return;
	}

	for (auto &m : materials)
	{
		int textureIdx = -1;
		if (!m.ambient_texname.empty())
		{
			std::string file = directory + m.ambient_texname;
			const char *path = file.c_str();

			textureIdx = MaterialManager::GetInstance()->AddTexture(path);
		}

		const glm::vec3 albedo = glm::vec3(m.diffuse[0], m.diffuse[1], m.diffuse[2]);
		const glm::vec3 specular = glm::vec3(m.specular[0], m.specular[1], m.specular[2]);
		const float transparency = glm::max(1.0f - m.dissolve, 0.0f);
		// Ns 10000  # ranges between 0 and 1000
		const float diffuse = glm::min(glm::max(1.f - (m.shininess / 1000), 0.f), 1.f);
		const float &refractIdx = m.ior;
		const glm::vec3 emission = glm::vec3(m.emission[0], m.emission[1], m.emission[2]);
		const glm::vec3 absorption = glm::vec3(m.transmittance[0], m.transmittance[1], m.transmittance[2]);

		Material mat;
		if (glm::any(glm::greaterThan(emission, glm::vec3(0))))
			mat = Material::light(emission);
		else if (transparency > 0.01f)
			mat = Material::fresnel(albedo, refractIdx, absorption, 0.0f, textureIdx);
		else if (diffuse < 0.99f)
			mat = Material::specular(specular, refractIdx);
		else
			mat = Material::lambertian(albedo, textureIdx);

		uint newMaterialIndex = MaterialManager::GetInstance()->AddMaterial(mat);
		fMaterialsIndices.push_back(newMaterialIndex);
	}

	for (tinyobj::shape_t &s : shapes)
	{
		// loop over faces
		int index_offset = 0;

		for (uint f = 0; f < s.mesh.num_face_vertices.size(); f++)
		{
			int fv = s.mesh.num_face_vertices[f];
			vec3 vertices[3];
			vec3 normals[3];
			vec2 textureCoordinates[3];

			bool meshHasNormals = !attrib.normals.empty();
			bool meshHasTextures = !attrib.texcoords.empty();

			for (int v = 0; v < fv; v++)
			{
				tinyobj::index_t idx = s.mesh.indices[index_offset + v];
				tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
				tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
				tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
				if (meshHasNormals)
				{
					tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
					tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
					tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
					normals[v] = vec3(nx, ny, nz);
				}
				if (meshHasTextures)
				{
					tinyobj::real_t tx = attrib.texcoords[2 * idx.texcoord_index + 0];
					tinyobj::real_t ty = attrib.texcoords[2 * idx.texcoord_index + 1];
					textureCoordinates[v] = vec2(tx, ty);
				}

				vertices[v] = vec3(vx, vy, vz);
			}

			unsigned int tMaterialIndex;

			if (!s.mesh.material_ids.empty() &&
				fMaterialsIndices.size() > static_cast<unsigned int>(s.mesh.material_ids[f]))
			{
				tMaterialIndex = fMaterialsIndices[s.mesh.material_ids[f]];
			}
			else
			{
				tMaterialIndex = matIndex;
			}

			const Material mat = MaterialManager::GetInstance()->GetMaterial(tMaterialIndex);
			Triangle *triangle;
			if (mat.textureIdx > -1 && meshHasTextures)
			{
				if (meshHasNormals)
				{
					triangle = new Triangle(vec3(transform * vec4(vertices[0] * scale, 1.0f)) + translation,
											vec3(transform * vec4(vertices[1] * scale, 1.0f)) + translation,
											vec3(transform * vec4(vertices[2] * scale, 1.0f)) + translation, normals[0],
											normals[1], normals[2], tMaterialIndex, textureCoordinates[0],
											textureCoordinates[1], textureCoordinates[2]);
				}
				else
				{
					triangle =
						new Triangle(vec3(transform * vec4(vertices[0] * scale, 1.0f)) + translation,
									 vec3(transform * vec4(vertices[1] * scale, 1.0f)) + translation,
									 vec3(transform * vec4(vertices[2] * scale, 1.0f)) + translation, tMaterialIndex,
									 textureCoordinates[0], textureCoordinates[1], textureCoordinates[2]);
				}
			}
			else
			{
				if (meshHasNormals)
				{
					triangle = new Triangle(vec3(transform * vec4(vertices[0] * scale, 1.0f)) + translation,
											vec3(transform * vec4(vertices[1] * scale, 1.0f)) + translation,
											vec3(transform * vec4(vertices[2] * scale, 1.0f)) + translation, normals[0],
											normals[1], normals[2], tMaterialIndex);
				}
				else
				{
					triangle =
						new Triangle(vec3(transform * vec4(vertices[0] * scale, 1.0f)) + translation,
									 vec3(transform * vec4(vertices[1] * scale, 1.0f)) + translation,
									 vec3(transform * vec4(vertices[2] * scale, 1.0f)) + translation, tMaterialIndex);
				}
			}

			if (mat.type == Light)
				objectList->AddLight(triangle);
			else
				objectList->AddObject(triangle);
			index_offset += fv;
		}
	}
}
