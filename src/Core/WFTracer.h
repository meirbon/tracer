#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "BVH/MBVHTree.h"
#include "BVH/StaticBVHTree.h"
#include "CL/Buffer.h"
#include "CL/OpenCL.h"
#include "Core/Camera.h"
#include "Core/PathTracer.h"
#include "GL/Texture.h"
#include "Primitives/TriangleList.h"
#include "Utils/Memory.h"

namespace core
{
class WFTracer : public Renderer
{
	struct TextureInfo
	{
		TextureInfo() = default;
		TextureInfo(int width, int height, int offset)
		{
			this->width = width;
			this->height = height;
			this->offset = offset;
			this->dummy = 0;
		}
		union {
			glm::ivec4 data{};
			struct
			{
				int width, height, offset, dummy;
			};
		};
	};

  public:
	WFTracer() = default;
	WFTracer(prims::TriangleList *tList, gl::Texture *t1, gl::Texture *t2, Camera *camera, Surface *skybox = nullptr,
			 ctpl::ThreadPool *pool = nullptr);
	~WFTracer() override;

	void Render(Surface *output) override;
	void Reset() override;

	inline void SetMode(int mode) override
	{
		cl::Kernel::syncQueue();
		m_Mode = mode;
	}

	inline void SetMode(const std::string &mode) override
	{
		for (uint i = 0; i < m_Modes.size(); i++)
		{
			if (mode == m_Modes[i])
			{
				m_Mode = i;
				break;
			}
		}
	}

	inline void SwitchSkybox() override
	{
		cl::Kernel::syncQueue();
		this->m_SkyboxEnabled = (this->m_HasSkybox && !this->m_SkyboxEnabled);
	}

	void Resize(int width, int height, gl::Texture *newOutput1, gl::Texture *newOutput2) override;

	void setupCamera();
	void setupObjects();
	void setupMaterials();
	void setupTextures();
	void setupSkybox(Surface *skybox);
	void setArguments();

  private:
	Camera *m_Camera = nullptr;
	bool m_SkyboxEnabled = false;
	bool m_HasSkybox = false;
	int m_Frame = 0, m_Width = 0, m_Height = 0;

	bvh::StaticBVHTree *m_BVHTree = nullptr;
	bvh::MBVHTree *m_MBVHTree = nullptr;
	gl::Texture *outputTexture[2] = {nullptr, nullptr};

	cl::TextureBuffer *outputBuffer[2] = {nullptr, nullptr};
	cl::Buffer<unsigned int> *primitiveIndicesBuffer = nullptr;
	cl::Buffer<bvh::MBVHNode> *MBVHNodeBuffer = nullptr;
	cl::Buffer<glm::vec4> *colorBuffer = nullptr;
	cl::Buffer<glm::vec4> *textureBuffer = nullptr;
	cl::Buffer<TextureInfo> *textureInfoBuffer = nullptr;
	cl::Buffer<glm::vec4> *skyDomeBuffer = nullptr;
	cl::Buffer<TextureInfo> *skyDomeInfoBuffer = nullptr;
	cl::Buffer<float> *raysBuffer = nullptr;
	cl::Buffer<float> *sRaysBuffer = nullptr;
	cl::Buffer<Material> *materialBuffer = nullptr;
	cl::Buffer<Microfacet> *microfacetBuffer = nullptr;
	cl::Buffer<unsigned int> *matIndicesBuffer = nullptr;
	cl::Buffer<glm::vec4> *verticesBuffer = nullptr;
	cl::Buffer<glm::vec4> *normalBuffer = nullptr;
	cl::Buffer<glm::vec2> *texCoordBuffer = nullptr;
	cl::Buffer<glm::vec4> *cnBuffer = nullptr;
	cl::Buffer<glm::uvec4> *indicesBuffer = nullptr;
	cl::Buffer<unsigned int> *lightIndicesBuffer = nullptr;
	cl::Kernel *wGenerateRayKernel = nullptr;
	cl::Kernel *wIntersectKernel = nullptr;
	cl::Kernel *wShadeRefKernel = nullptr;
	cl::Kernel *wShadeNeeKernel = nullptr;
	cl::Kernel *wShadeMisKernel = nullptr;
	cl::Kernel *wConnectKernel = nullptr;
	cl::Kernel *wDrawKernel = nullptr;

	prims::TriangleList *m_TList = nullptr;

	int tIdx = 0;
};
} // namespace core
