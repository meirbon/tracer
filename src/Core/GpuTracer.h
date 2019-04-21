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
#include "Primitives/GpuTriangleList.h"
#include "Utils/Memory.h"

using namespace cl;

namespace prims
{
class SceneObjectList;
}

namespace core
{
struct TextureInfo
{
	union {
		glm::vec<4, int> data;
		struct
		{
			int width, height, offset, dummy;
		};
	};
	TextureInfo(int width, int height, int offset)
	{
		this->width = width;
		this->height = height;
		this->offset = offset;
	}
};

class GpuTracer : public Renderer
{
  public:
	GpuTracer() = default;
	GpuTracer(prims::GpuTriangleList *objectList, gl::Texture *targetTexture1, gl::Texture *targetTexture2,
			  Camera *camera, Surface *skyBox = nullptr, ctpl::ThreadPool *pool = nullptr);
	~GpuTracer() override;

	void Render(Surface *output) override;

	void Reset() override;

	inline void SetMode(Mode mode) override
	{
		cl::Kernel::SyncQueue();
		m_Mode = mode;
//		intersectRaysKernelRef->SetArgument(18, m_Mode);
//		intersectRaysKernelOpt->SetArgument(18, m_Mode);
//		intersectRaysKernelBVH->SetArgument(18, m_Mode);
//		intersectRaysKernelMF->SetArgument(18, m_Mode);
	}

	inline void SwitchSkybox() override
	{
		cl::Kernel::SyncQueue();
		this->m_SkyboxEnabled = (this->m_HasSkybox && !this->m_SkyboxEnabled);
//		intersectRaysKernelRef->SetArgument(15, m_SkyboxEnabled);
//		intersectRaysKernelOpt->SetArgument(15, m_SkyboxEnabled);
//		intersectRaysKernelBVH->SetArgument(15, m_SkyboxEnabled);
//		intersectRaysKernelMF->SetArgument(15, m_SkyboxEnabled);
	}

	void Resize(gl::Texture *newOutput) override;

	void SetupCamera();
	void SetupSeeds(int width, int height);

	void SetupObjects();
	void SetupMaterials();

	void SetupNEEData();
	void SetupTextures();
	void SetupSkybox(Surface *skyBox);

	void SetArguments();

	inline static unsigned int UpperPowerOfTwo(unsigned int v)
	{
		return static_cast<unsigned int>(pow(2, ceil(log(v) / log(2))));
	}

	inline const char *GetModeString() const noexcept override
	{
		switch (m_Mode)
		{
		case (NEE_MIS):
			return "NEE MIS";
		case (Reference):
			return "Ref";
		case (NEE):
		case (IS):
		case (NEE_IS):
		case (NEEMicrofacet):
		case (ReferenceMicrofacet):
		default:
			return "Ref MF";
		}
	}

	inline int GetSamples() const override { return m_Samples; }

	inline void SetOutput(gl::Texture *outputTexture1, gl::Texture *outputTexture2)
	{
		this->outputTexture[0] = outputTexture1;
		this->outputTexture[1] = outputTexture2;
	}

	inline void SetMode(std::string mode) override
	{
		if (mode == "NEE MIS")
			SetMode(NEE_MIS);
		else if (mode == "Reference")
			SetMode(Reference);
		else if (mode == "Reference MF")
			SetMode(ReferenceMicrofacet);
	};

  private:
	Camera *m_Camera = nullptr;
	prims::GpuTriangleList *m_ObjectList = nullptr;
	bvh::StaticBVHTree *m_BVHTree = nullptr;
	bvh::MBVHTree *m_MBVHTree = nullptr;
	gl::Texture *outputTexture[2] = {nullptr, nullptr};
	cl::Buffer *outputBuffer = nullptr;

	cl::Buffer *primitiveIndicesBuffer = nullptr;
	cl::Buffer *BVHNodeBuffer = nullptr;
	cl::Buffer *MBVHNodeBuffer = nullptr;
	cl::Buffer *seedBuffer = nullptr;
	cl::Buffer *previousColorBuffer = nullptr;
	cl::Buffer *colorBuffer = nullptr;
	cl::Buffer *textureBuffer = nullptr;
	cl::Buffer *textureInfoBuffer = nullptr;
	cl::Buffer *skyDome = nullptr;
	cl::Buffer *skyDomeInfo = nullptr;

	cl::Buffer *raysBuffer = nullptr;
	cl::Buffer *cameraBuffer = nullptr;
	cl::Buffer *materialBuffer = nullptr;
	cl::Buffer *microfacetBuffer = nullptr;
	cl::Buffer *triangleBuffer = nullptr;

	cl::Buffer *lightIndices = nullptr;
	cl::Buffer *lightLotteryTickets = nullptr;
	int lightCount{};
	float lightArea{};

	cl::Kernel *generateRayKernel = nullptr;
	cl::Kernel *intersectRaysKernelRef = nullptr;
	cl::Kernel *intersectRaysKernelMF = nullptr;
	cl::Kernel *intersectRaysKernelBVH = nullptr;
	cl::Kernel *intersectRaysKernelOpt = nullptr;
	cl::Kernel *drawKernel = nullptr;

	cl::Kernel *wGenerateRayKernel = nullptr;
	cl::Kernel *wIntersectKernel = nullptr;
	cl::Kernel *wShadeKernel = nullptr;
	cl::Kernel *wDrawKernel = nullptr;

	uint frame = 0;
	int tIndex = 0;
	int m_Samples = 0;
	int m_Width{}, m_Height{};

	bool m_SkyboxEnabled{};
	bool m_HasSkybox = false;
}; // namespace core
} // namespace core