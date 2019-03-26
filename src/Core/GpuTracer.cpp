#include "Core/GpuTracer.h"
#include "Core/Surface.h"
#include "Shared.h"

#define WF 0

using namespace cl;
using namespace gl;
using namespace core;

namespace core
{

struct GpuSphere
{
	glm::vec3 centroid;
	uint materialIdx;
	float radiusSquared;
	vec3 padding;
};

struct GpuCamera
{
	glm::vec3 origin; // 16
	float dummy;
	glm::vec3 viewDirection; // 32
	float dummy2;

	float viewDistance; // 36
	float invWidth;		// 40

	float invHeight;   // 44
	float aspectRatio; // 48
};

inline unsigned int RoundToPowerOf2(unsigned int v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

GpuTracer::GpuTracer(prims::GpuTriangleList *objectList, gl::Texture *targetTexture1, gl::Texture *targetTexture2,
					 core::Camera *camera, core::Surface *skyBox, ctpl::ThreadPool *pool)
	: m_Camera(camera)
{
	modes = {"NEE MIS", "Reference", "Reference MF"};
	Kernel::InitCL();
	m_BVHTree = new bvh::StaticBVHTree(objectList, bvh::BVHType::SAH_BINNING, pool);
	m_BVHTree->ConstructBVH();
	m_MBVHTree = new bvh::MBVHTree(m_BVHTree);

	m_ObjectList = objectList;
	outputTexture[0] = targetTexture1;
	outputTexture[1] = targetTexture2;

	const int width = targetTexture1->GetWidth();
	const int height = targetTexture1->GetHeight();

	m_Width = width;
	m_Height = height;

	const auto w = UpperPowerOfTwo(width);
	const auto h = UpperPowerOfTwo(height);

	auto workSize = std::tuple<size_t, size_t, size_t>(w, h, 1);
	auto localSize = std::tuple<size_t, size_t, size_t>(8, 8, 1);

	generateRayKernel = new Kernel("programs/program.cl", "generateRays", workSize, localSize);
	intersectRaysKernelRef = new Kernel("programs/program.cl", "intersectRays", workSize, localSize);
	intersectRaysKernelMF = new Kernel("programs/program.cl", "intersectRaysMF", workSize, localSize);
	intersectRaysKernelBVH = new Kernel("programs/program.cl", "intersectRaysBVH", workSize, localSize);
	intersectRaysKernelOpt = new Kernel("programs/program.cl", "intersectRaysOpt", workSize, localSize);
	drawKernel = new Kernel("programs/program.cl", "Draw", workSize, localSize);

	wIntersectKernel = new Kernel("programs/program.cl", "intersect", workSize, localSize);
	wShadeKernel = new Kernel("programs/program.cl", "shade", workSize, localSize);
	wDrawKernel = new Kernel("programs/program.cl", "draw", workSize, localSize);

	this->Resize(targetTexture1);

	this->SetupObjects();
	this->SetupMaterials();

	this->SetupTextures();
	this->SetupNEEData();
	this->SetupSkybox(skyBox);

	SetArguments();
}

GpuTracer::~GpuTracer()
{
	delete m_BVHTree;
	delete m_MBVHTree;
	delete generateRayKernel;
	delete intersectRaysKernelRef;
	delete intersectRaysKernelMF;
	delete intersectRaysKernelBVH;
	delete intersectRaysKernelOpt;
	delete outputBuffer;
	delete raysBuffer;
	delete BVHNodeBuffer;
	delete MBVHNodeBuffer;
	delete primitiveIndicesBuffer;
	delete seedBuffer;
	delete colorBuffer;
	delete textureBuffer;
	delete textureInfoBuffer;
	delete previousColorBuffer;

	delete cameraBuffer;
	delete materialBuffer;
	delete triangleBuffer;

	delete lightIndices;
	delete lightLotteryTickets;

	delete wIntersectKernel;
	delete wDrawKernel;
}

void GpuTracer::Render(Surface *)
{
	intersectRaysKernelRef->SetArgument(18, m_Width);
	intersectRaysKernelRef->SetArgument(19, m_Height);
	intersectRaysKernelMF->SetArgument(18, m_Width);
	intersectRaysKernelMF->SetArgument(19, m_Height);
	intersectRaysKernelBVH->SetArgument(18, m_Width);
	intersectRaysKernelBVH->SetArgument(19, m_Height);
	intersectRaysKernelOpt->SetArgument(18, m_Width);
	intersectRaysKernelOpt->SetArgument(19, m_Height);

	drawKernel->SetArgument(3, m_Samples);
	wDrawKernel->SetArgument(4, m_Samples);

	generateRayKernel->Run();
#if WF
	for (int i = 0; i < 8; i++)
	{
		wIntersectKernel->Run();
		wShadeKernel->Run();
	}
	wDrawKernel->Run();
#else
	switch (m_Mode)
	{
	case (Mode::Reference):
		intersectRaysKernelRef->Run();
		break;
	case (Mode::NEE_MIS):
		intersectRaysKernelOpt->Run();
		break;
	case (Mode::ReferenceMicrofacet):
	default:
		intersectRaysKernelMF->Run();
		break;
	}
	drawKernel->Run(outputBuffer);
#endif

	m_Samples++;

	// update camera
	this->SetupCamera();

	// calculate on CPU to make sure we only calculate it once
	const glm::vec3 u = normalize(cross(m_Camera->GetViewDirection(), vec3(0, 1, 0)));
	const glm::vec3 v = normalize(cross(u, m_Camera->GetViewDirection()));
	const glm::vec3 horizontal = u * m_Camera->GetFOVDistance() * m_Camera->GetAspectRatio();
	const glm::vec3 vertical = v * m_Camera->GetFOVDistance();

	generateRayKernel->SetArgument(3, vec4(horizontal, 1.0f));
	generateRayKernel->SetArgument(4, vec4(vertical, 1.0f));
}

void GpuTracer::Reset()
{
	Kernel::SyncQueue();
	m_Samples = 0;
}

void GpuTracer::Resize(Texture *newOutput)
{
	outputTexture[0] = newOutput;

	const int width = newOutput->GetWidth();
	const int height = newOutput->GetHeight();

	m_Width = width;
	m_Height = height;

	delete outputBuffer;
	delete raysBuffer;
	delete previousColorBuffer;
	delete colorBuffer;
	delete cameraBuffer;

	outputBuffer = nullptr;
	raysBuffer = nullptr;
	previousColorBuffer = nullptr;
	colorBuffer = nullptr;
	cameraBuffer = nullptr;

	SetupCamera();

	const auto roundedWidth = RoundToPowerOf2(width);
	const auto roundedHeight = RoundToPowerOf2(height);

	// set up textures
	outputBuffer = new Buffer(newOutput, BufferType::TARGET);

	// create buffer to store primary ray
	raysBuffer = new Buffer(width * height * 48);

	// create buffers to store colors for accumulation
	previousColorBuffer = new Buffer(width * height * sizeof(glm::vec4));
	colorBuffer = new Buffer(width * height * sizeof(glm::vec4));

	generateRayKernel->SetWorkSize(roundedWidth, roundedHeight, 1);
	intersectRaysKernelRef->SetWorkSize(roundedWidth, roundedHeight, 1);
	intersectRaysKernelMF->SetWorkSize(roundedWidth, roundedHeight, 1);
	intersectRaysKernelBVH->SetWorkSize(roundedWidth, roundedHeight, 1);
	intersectRaysKernelOpt->SetWorkSize(roundedWidth, roundedHeight, 1);
	drawKernel->SetWorkSize(roundedWidth, roundedHeight, 1);

	SetupSeeds(width, height);

	SetArguments();
}

void GpuTracer::SetupCamera()
{
	// create a buffer for the camera
	if (cameraBuffer == nullptr)
		cameraBuffer = new Buffer(sizeof(GpuCamera));

	auto *cam = cameraBuffer->GetHostPtr<GpuCamera>();
	cam->origin = m_Camera->GetPosition();
	cam->viewDirection = m_Camera->GetViewDirection();
	cam->viewDistance = m_Camera->GetFOVDistance();
	cam->invWidth = m_Camera->GetInvWidth();
	cam->invHeight = m_Camera->GetInvHeight();
	cam->aspectRatio = m_Camera->GetAspectRatio();

	cameraBuffer->CopyToDevice();
}

void GpuTracer::SetupSeeds(int width, int height)
{
	delete seedBuffer;

	// https://en.cppreference.com/w/cpp/numeric/random/uniform_int_distribution
	std::random_device rd;  // Will be used to obtain a seed for the random number engine
	std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
	std::vector<unsigned int> seeds{};

	seeds.resize(width * height);
	for (int i = 0; i < width * height; i++)
	{
		seeds[i] = gen();
	}
	seedBuffer = new Buffer(width * height * sizeof(unsigned int), seeds.data());
	seedBuffer->CopyToDevice();
}

void GpuTracer::SetupObjects()
{
	// copy initial BVH tree to GPU
	primitiveIndicesBuffer = new Buffer(static_cast<unsigned int>(m_BVHTree->m_PrimitiveCount) * sizeof(unsigned int),
										m_BVHTree->m_PrimitiveIndices.data());
	primitiveIndicesBuffer->CopyToDevice();

	// create kernels
#if MBVH
	BVHNodeBuffer = new Buffer(sizeof(bvh::BVHNode), m_BVHTree->m_BVHPool.data());
#else
	BVHNodeBuffer = new Buffer(m_BVHTree->m_BVHPool.size() * sizeof(BVHNode), m_BVHTree->m_BVHPool.data());
#endif
	BVHNodeBuffer->CopyToDevice();

	MBVHNodeBuffer = new Buffer((m_MBVHTree->m_FinalPtr + 1) * sizeof(bvh::MBVHNode), m_MBVHTree->m_Tree.data());
	MBVHNodeBuffer->CopyToDevice();

	triangleBuffer =
		new Buffer(static_cast<unsigned int>(m_ObjectList->GetTriangles().size()) * sizeof(prims::GpuTriangle),
				   (void *)m_ObjectList->GetTriangles().data());
	triangleBuffer->CopyToDevice();
}

void GpuTracer::SetupMaterials()
{
	materialBuffer =
		new Buffer(static_cast<unsigned int>(MaterialManager::GetInstance()->GetMaterials().size()) * sizeof(Material),
				   (void *)MaterialManager::GetInstance()->GetMaterials().data());
	materialBuffer->CopyToDevice();

	microfacetBuffer = new Buffer(static_cast<unsigned int>(MaterialManager::GetInstance()->GetMicrofacets().size()) *
									  sizeof(Microfacet),
								  (void *)MaterialManager::GetInstance()->GetMicrofacets().data());
	microfacetBuffer->CopyToDevice();
}

void GpuTracer::SetupNEEData()
{
	// NEE
	lightCount = static_cast<int>(m_ObjectList->GetLightIndices().size());
	lightArea = 0.f;
	if (lightCount > 0)
	{
		lightIndices = new Buffer(lightCount * sizeof(unsigned int), &m_ObjectList->GetLightIndices()[0]);
		lightIndices->CopyToDevice();
		for (auto &i : m_ObjectList->GetLightIndices())
		{
			auto &t = m_ObjectList->GetTriangle(i);
			lightArea += t.m_Area;
		}

		std::vector<float> lLotteryTickets{};
		lLotteryTickets.resize(lightCount);
		lLotteryTickets[0] = m_ObjectList->GetTriangle(0).m_Area / lightArea;
		for (int i = 1; i < lightCount; i++)
		{
			lLotteryTickets[i] = lLotteryTickets[i - 1] + m_ObjectList->GetTriangle(i).m_Area / lightArea;
		}

		if (lightCount != 0)
		{
			lLotteryTickets[lightCount - 1] = 1.f;
		}

		lightLotteryTickets = new Buffer(lightCount * sizeof(float), lLotteryTickets.data());
		lightLotteryTickets->CopyToDevice();
	}
	else
	{
		lightLotteryTickets = new Buffer(4);
		lightIndices = new Buffer(4);
	}
}

void GpuTracer::SetupTextures()
{
	delete textureBuffer;
	delete textureInfoBuffer;

	if (!MaterialManager::GetInstance()->GetTextures().empty())
	{
		std::vector<TextureInfo> textureInfos{};
		unsigned int vec4Offset = 0;
		for (auto &tex : MaterialManager::GetInstance()->GetTextures())
		{
			const auto texSize = tex->GetWidth() * tex->GetHeight();
			textureInfos.emplace_back(tex->GetWidth(), tex->GetHeight(), vec4Offset);
			vec4Offset += texSize;
		}

		auto *textureColors = new glm::vec4[vec4Offset];
		for (unsigned int i = 0; i < textureInfos.size(); i++)
		{
			const int offset = textureInfos[i].offset;

			auto tex = MaterialManager::GetInstance()->GetTextures()[i];
			memcpy(&textureColors[offset], tex->GetTextureBuffer(), tex->GetWidth() * tex->GetHeight() * sizeof(vec4));
		}

		textureBuffer = new Buffer(vec4Offset * sizeof(vec4), textureColors);
		textureBuffer->CopyToDevice();
		textureInfoBuffer =
			new Buffer(static_cast<unsigned int>(textureInfos.size()) * sizeof(TextureInfo), textureInfos.data());
		textureInfoBuffer->CopyToDevice();

		delete[] textureColors;
	}
}

void GpuTracer::SetupSkybox(Surface *skyBox)
{
	delete skyDome;
	delete skyDomeInfo;

	m_HasSkybox = (skyBox != nullptr);
	if (m_HasSkybox)
	{
		auto skyInfo = TextureInfo(skyBox->GetWidth(), skyBox->GetHeight(), 0);
		skyDome = new Buffer(skyBox->GetWidth() * skyBox->GetHeight() * sizeof(glm::vec4), skyBox->GetTextureBuffer());
		skyDomeInfo = new Buffer(sizeof(TextureInfo), &skyInfo);
		skyDome->CopyToDevice();
		skyDomeInfo->CopyToDevice();
	}
	else
	{
		skyDome = new Buffer(4);
		skyDomeInfo = new Buffer(4);
	}
}

void GpuTracer::SetArguments()
{
	// set initial arguments for kernels
	generateRayKernel->SetArgument(0, raysBuffer);
	generateRayKernel->SetArgument(1, cameraBuffer);
	generateRayKernel->SetArgument(2, seedBuffer);

	const glm::vec3 u = normalize(cross(m_Camera->GetViewDirection(), vec3(0, 1, 0)));
	const glm::vec3 v = normalize(cross(u, m_Camera->GetViewDirection()));
	const glm::vec3 horizontal = u * m_Camera->GetFOVDistance() * m_Camera->GetAspectRatio();
	const glm::vec3 vertical = v * m_Camera->GetFOVDistance();

	generateRayKernel->SetArgument(3, vec4(horizontal, 1.0f));
	generateRayKernel->SetArgument(4, vec4(vertical, 1.0f));
	generateRayKernel->SetArgument(5, m_Width);
	generateRayKernel->SetArgument(6, m_Height);

	intersectRaysKernelRef->SetArgument(0, raysBuffer);
	intersectRaysKernelRef->SetArgument(1, materialBuffer);
	intersectRaysKernelRef->SetArgument(2, triangleBuffer);
	intersectRaysKernelRef->SetArgument(3, BVHNodeBuffer);
	intersectRaysKernelRef->SetArgument(4, MBVHNodeBuffer);
	intersectRaysKernelRef->SetArgument(5, primitiveIndicesBuffer);
	intersectRaysKernelRef->SetArgument(6, seedBuffer);
	intersectRaysKernelRef->SetArgument(7, colorBuffer);
	intersectRaysKernelRef->SetArgument(8, lightIndices);
	intersectRaysKernelRef->SetArgument(9, lightLotteryTickets);
	intersectRaysKernelRef->SetArgument(10, textureBuffer);
	intersectRaysKernelRef->SetArgument(11, textureInfoBuffer);
	intersectRaysKernelRef->SetArgument(12, skyDome);
	intersectRaysKernelRef->SetArgument(13, skyDomeInfo);
	intersectRaysKernelRef->SetArgument(14, microfacetBuffer);
	intersectRaysKernelRef->SetArgument(15, m_HasSkybox);
	intersectRaysKernelRef->SetArgument(16, lightArea);
	intersectRaysKernelRef->SetArgument(17, lightCount);
	intersectRaysKernelRef->SetArgument(18, m_Width);
	intersectRaysKernelRef->SetArgument(19, m_Height);

	intersectRaysKernelOpt->SetArgument(0, raysBuffer);
	intersectRaysKernelOpt->SetArgument(1, materialBuffer);
	intersectRaysKernelOpt->SetArgument(2, triangleBuffer);
	intersectRaysKernelOpt->SetArgument(3, BVHNodeBuffer);
	intersectRaysKernelOpt->SetArgument(4, MBVHNodeBuffer);
	intersectRaysKernelOpt->SetArgument(5, primitiveIndicesBuffer);
	intersectRaysKernelOpt->SetArgument(6, seedBuffer);
	intersectRaysKernelOpt->SetArgument(7, colorBuffer);
	intersectRaysKernelOpt->SetArgument(8, lightIndices);
	intersectRaysKernelOpt->SetArgument(9, lightLotteryTickets);
	intersectRaysKernelOpt->SetArgument(10, textureBuffer);
	intersectRaysKernelOpt->SetArgument(11, textureInfoBuffer);
	intersectRaysKernelOpt->SetArgument(12, skyDome);
	intersectRaysKernelOpt->SetArgument(13, skyDomeInfo);
	intersectRaysKernelOpt->SetArgument(14, microfacetBuffer);
	intersectRaysKernelOpt->SetArgument(15, m_HasSkybox);
	intersectRaysKernelOpt->SetArgument(16, lightArea);
	intersectRaysKernelOpt->SetArgument(17, lightCount);
	intersectRaysKernelOpt->SetArgument(18, m_Width);
	intersectRaysKernelOpt->SetArgument(19, m_Height);

	intersectRaysKernelBVH->SetArgument(0, raysBuffer);
	intersectRaysKernelBVH->SetArgument(1, materialBuffer);
	intersectRaysKernelBVH->SetArgument(2, triangleBuffer);
	intersectRaysKernelBVH->SetArgument(3, BVHNodeBuffer);
	intersectRaysKernelBVH->SetArgument(4, MBVHNodeBuffer);
	intersectRaysKernelBVH->SetArgument(5, primitiveIndicesBuffer);
	intersectRaysKernelBVH->SetArgument(6, seedBuffer);
	intersectRaysKernelBVH->SetArgument(7, colorBuffer);
	intersectRaysKernelBVH->SetArgument(8, lightIndices);
	intersectRaysKernelBVH->SetArgument(9, lightLotteryTickets);
	intersectRaysKernelBVH->SetArgument(10, textureBuffer);
	intersectRaysKernelBVH->SetArgument(11, textureInfoBuffer);
	intersectRaysKernelBVH->SetArgument(12, skyDome);
	intersectRaysKernelBVH->SetArgument(13, skyDomeInfo);
	intersectRaysKernelBVH->SetArgument(14, microfacetBuffer);
	intersectRaysKernelBVH->SetArgument(15, m_HasSkybox);
	intersectRaysKernelBVH->SetArgument(16, lightArea);
	intersectRaysKernelBVH->SetArgument(17, lightCount);
	intersectRaysKernelBVH->SetArgument(18, m_Width);
	intersectRaysKernelBVH->SetArgument(19, m_Height);

	intersectRaysKernelMF->SetArgument(0, raysBuffer);
	intersectRaysKernelMF->SetArgument(1, materialBuffer);
	intersectRaysKernelMF->SetArgument(2, triangleBuffer);
	intersectRaysKernelMF->SetArgument(3, BVHNodeBuffer);
	intersectRaysKernelMF->SetArgument(4, MBVHNodeBuffer);
	intersectRaysKernelMF->SetArgument(5, primitiveIndicesBuffer);
	intersectRaysKernelMF->SetArgument(6, seedBuffer);
	intersectRaysKernelMF->SetArgument(7, colorBuffer);
	intersectRaysKernelMF->SetArgument(8, lightIndices);
	intersectRaysKernelMF->SetArgument(9, lightLotteryTickets);
	intersectRaysKernelMF->SetArgument(10, textureBuffer);
	intersectRaysKernelMF->SetArgument(11, textureInfoBuffer);
	intersectRaysKernelMF->SetArgument(12, skyDome);
	intersectRaysKernelMF->SetArgument(13, skyDomeInfo);
	intersectRaysKernelMF->SetArgument(14, microfacetBuffer);
	intersectRaysKernelMF->SetArgument(15, m_HasSkybox);
	intersectRaysKernelMF->SetArgument(16, lightArea);
	intersectRaysKernelMF->SetArgument(17, lightCount);
	intersectRaysKernelMF->SetArgument(18, m_Width);
	intersectRaysKernelMF->SetArgument(19, m_Height);

	drawKernel->SetArgument(0, outputBuffer);
	drawKernel->SetArgument(1, previousColorBuffer);
	drawKernel->SetArgument(2, colorBuffer);
	drawKernel->SetArgument(3, m_Samples);
	drawKernel->SetArgument(4, m_Width);
	drawKernel->SetArgument(5, m_Height);

	wIntersectKernel->SetArgument(0, raysBuffer);
	wIntersectKernel->SetArgument(1, materialBuffer);
	wIntersectKernel->SetArgument(2, triangleBuffer);
	wIntersectKernel->SetArgument(3, BVHNodeBuffer);
	wIntersectKernel->SetArgument(4, MBVHNodeBuffer);
	wIntersectKernel->SetArgument(5, primitiveIndicesBuffer);
	wIntersectKernel->SetArgument(6, seedBuffer);
	wIntersectKernel->SetArgument(7, colorBuffer);
	wIntersectKernel->SetArgument(8, lightIndices);
	wIntersectKernel->SetArgument(9, lightLotteryTickets);
	wIntersectKernel->SetArgument(10, textureBuffer);
	wIntersectKernel->SetArgument(11, textureInfoBuffer);
	wIntersectKernel->SetArgument(12, skyDome);
	wIntersectKernel->SetArgument(13, skyDomeInfo);
	wIntersectKernel->SetArgument(14, microfacetBuffer);
	wIntersectKernel->SetArgument(15, m_HasSkybox);
	wIntersectKernel->SetArgument(16, lightArea);
	wIntersectKernel->SetArgument(17, lightCount);
	wIntersectKernel->SetArgument(18, m_Width);
	wIntersectKernel->SetArgument(19, m_Height);

	wShadeKernel->SetArgument(0, raysBuffer);
	wShadeKernel->SetArgument(1, materialBuffer);
	wShadeKernel->SetArgument(2, triangleBuffer);
	wShadeKernel->SetArgument(3, BVHNodeBuffer);
	wShadeKernel->SetArgument(4, MBVHNodeBuffer);
	wShadeKernel->SetArgument(5, primitiveIndicesBuffer);
	wShadeKernel->SetArgument(6, seedBuffer);
	wShadeKernel->SetArgument(7, colorBuffer);
	wShadeKernel->SetArgument(8, lightIndices);
	wShadeKernel->SetArgument(9, lightLotteryTickets);
	wShadeKernel->SetArgument(10, textureBuffer);
	wShadeKernel->SetArgument(11, textureInfoBuffer);
	wShadeKernel->SetArgument(12, skyDome);
	wShadeKernel->SetArgument(13, skyDomeInfo);
	wShadeKernel->SetArgument(14, microfacetBuffer);
	wShadeKernel->SetArgument(15, m_HasSkybox);
	wShadeKernel->SetArgument(16, lightArea);
	wShadeKernel->SetArgument(17, lightCount);
	wShadeKernel->SetArgument(18, m_Width);
	wShadeKernel->SetArgument(19, m_Height);

	wDrawKernel->SetArgument(0, outputBuffer);
	wDrawKernel->SetArgument(1, raysBuffer);
	wDrawKernel->SetArgument(2, previousColorBuffer);
	wDrawKernel->SetArgument(3, colorBuffer);
	wDrawKernel->SetArgument(4, m_Samples);
	wDrawKernel->SetArgument(5, m_Width);
	wDrawKernel->SetArgument(6, m_Height);
}
} // namespace core