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
  public:
	WFTracer() = default;
	WFTracer(TriangleList *tList, gl::Texture *t1, gl::Texture *t2, Camera *camera, Surface *skybox = nullptr,
			 ctpl::ThreadPool *pool = nullptr);
	~WFTracer() override;

	void Render(Surface *output) override;
	void Reset() override;

	inline void SetMode(int mode) override
	{
		cl::Kernel::SyncQueue();
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
		cl::Kernel::SyncQueue();
		this->m_SkyboxEnabled = (this->m_HasSkybox && !this->m_SkyboxEnabled);
	}

	void Resize(gl::Texture *newOutput) override;

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

	cl::Buffer *outputBuffer = nullptr;
	cl::Buffer *primitiveIndicesBuffer = nullptr;
	cl::Buffer *MBVHNodeBuffer = nullptr;
	cl::Buffer *colorBuffer = nullptr;
	cl::Buffer *textureBuffer = nullptr;
	cl::Buffer *textureInfoBuffer = nullptr;
	cl::Buffer *skyDomeBuffer = nullptr;
	cl::Buffer *skyDomeInfoBuffer = nullptr;
	cl::Buffer *raysBuffer = nullptr;
	cl::Buffer *sRaysBuffer = nullptr;
	cl::Buffer *materialBuffer = nullptr;
	cl::Buffer *microfacetBuffer = nullptr;
	cl::Buffer *matIndicesBuffer = nullptr;
	cl::Buffer *verticesBuffer = nullptr;
	cl::Buffer *normalBuffer = nullptr;
	cl::Buffer *texCoordBuffer = nullptr;
	cl::Buffer *cnBuffer = nullptr;
	cl::Buffer *indicesBuffer = nullptr;
	cl::Buffer *lightIndicesBuffer = nullptr;
	cl::Kernel *wGenerateRayKernel = nullptr;
	cl::Kernel *wIntersectKernel = nullptr;
	cl::Kernel *wShadeRefKernel = nullptr;
	cl::Kernel *wShadeNeeKernel = nullptr;
	cl::Kernel *wShadeMisKernel = nullptr;
	cl::Kernel *wConnectKernel = nullptr;
	cl::Kernel *wDrawKernel = nullptr;

	TriangleList *m_TList = nullptr;
};
} // namespace core
