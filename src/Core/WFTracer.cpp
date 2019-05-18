#include "WFTracer.h"

#include <future>

using namespace cl;
using namespace gl;
using namespace core;

namespace core
{

WFTracer::WFTracer(prims::TriangleList *tList, gl::Texture *t1, gl::Texture *t2, Camera *camera, Surface *skybox,
				   ctpl::ThreadPool *pool)
	: m_Camera(camera), m_Frame(0), m_TList(tList)
{
	m_Modes = {"Ref", "Nee", "Mis", "RT"};
	Kernel::initCL();
	m_BVHTree = new bvh::StaticBVHTree(tList->m_AABBs, bvh::BVHType::SAH_BINNING, pool);
	m_BVHTree->ConstructBVH();
	m_MBVHTree = new bvh::MBVHTree(m_BVHTree);

	outputTexture[0] = t1;
	outputTexture[1] = t2;

	this->m_Width = t1->GetWidth();
	this->m_Height = t1->GetHeight();

	auto workSize = std::tuple<size_t, size_t, size_t>(m_Width, m_Height, 1);

	wGenerateRayKernel = new Kernel("cl-src/program.cl", "generate", workSize);
	wIntersectKernel = new Kernel("cl-src/program.cl", "intersect", workSize);
	wShadeRefKernel = new Kernel("cl-src/shade_ref.cl", "shade_ref", workSize);
	wShadeNeeKernel = new Kernel("cl-src/shade_nee.cl", "shade_nee", workSize);
	wShadeMisKernel = new Kernel("cl-src/shade_mis.cl", "shade_mis", workSize);
	wShadeRayTracer = new Kernel("cl-src/shade_raytracer.cl", "shade_raytracer", workSize);
	wConnectKernel = new Kernel("cl-src/program.cl", "connect", workSize);
	wDrawKernel = new Kernel("cl-src/program.cl", "draw", workSize);

	Resize(m_Width, m_Height, t1, t2);
	setupObjects();
	setupMaterials();
	setupTextures();
	setupSkybox(skybox);
	setArguments();
	colorBuffer->clear();
}

WFTracer::~WFTracer()
{
	delete m_BVHTree;
	delete m_MBVHTree;

	delete wGenerateRayKernel;
	delete wIntersectKernel;
	delete wShadeRefKernel;
	delete wShadeNeeKernel;
	delete wShadeMisKernel;
	delete wShadeRayTracer;
	delete wConnectKernel;
	delete wDrawKernel;
}

void WFTracer::Render(Surface *)
{
	m_Frame = glm::max(m_Frame, 1);

	CHECKCL(wDrawKernel->setBuffer(0, outputBuffer[tIdx]));
	CHECKCL(wGenerateRayKernel->setArgument(6, 1, &m_Frame));
	CHECKCL(wShadeRefKernel->setArgument(21, 1, &m_Frame));
	CHECKCL(wShadeNeeKernel->setArgument(21, 1, &m_Frame));
	CHECKCL(wShadeMisKernel->setArgument(21, 1, &m_Frame));
	CHECKCL(wShadeRayTracer->setArgument(21, 1, &m_Frame));
	CHECKCL(wDrawKernel->setArgument(4, 1, &m_Frame));

	CHECKCL(wGenerateRayKernel->run());
	auto result = m_Camera->updateGPUBufferAsync();
	CHECKCL(wIntersectKernel->run());

	switch (m_Mode)
	{
	case (1):
		CHECKCL(wShadeNeeKernel->run());
		CHECKCL(wConnectKernel->run());
		break;
	case (2):
		CHECKCL(wShadeMisKernel->run());
		CHECKCL(wConnectKernel->run());
		break;
	case (3):
		CHECKCL(wShadeRayTracer->run());
		CHECKCL(wConnectKernel->run());
	case (0):
	default:
		CHECKCL(wShadeRefKernel->run());
		break;
	}

	CHECKCL(wDrawKernel->run(outputBuffer[tIdx]));

	tIdx = 1 - tIdx;
	m_Samples++;
	m_Frame++;
	result.get();
}

void WFTracer::Reset()
{
	m_Samples = 0;
	m_Frame = 1;
	Kernel::syncQueue();
}

void WFTracer::Resize(int width, int height, gl::Texture *newOutput1, gl::Texture *newOutput2)
{
	outputTexture[0] = newOutput1;
	outputTexture[1] = newOutput2;
	m_Width = width;
	m_Height = height;
	setupCamera();

	const auto sizeReqX = wGenerateRayKernel->getLocalSize()[0];
	const auto sizeReqY = wGenerateRayKernel->getLocalSize()[1];

	auto workSizeX = size_t(glm::ceil(float(m_Width) / float(sizeReqX))) * sizeReqX;
	auto workSizeY = size_t(glm::ceil(float(m_Height) / float(sizeReqY))) * sizeReqY;

	wGenerateRayKernel->setWorkSize(workSizeX, workSizeY, 1);
	wIntersectKernel->setWorkSize(workSizeX, workSizeY, 1);
	wShadeRefKernel->setWorkSize(workSizeX, workSizeY, 1);
	wShadeNeeKernel->setWorkSize(workSizeX, workSizeY, 1);
	wShadeMisKernel->setWorkSize(workSizeX, workSizeY, 1);
	wShadeRayTracer->setWorkSize(workSizeX, workSizeY, 1);
	wConnectKernel->setWorkSize(workSizeX, workSizeY, 1);
	wDrawKernel->setWorkSize(workSizeX, workSizeY, 1);

	delete outputBuffer[0];
	delete outputBuffer[1];

	outputBuffer[0] = new TextureBuffer(newOutput1, BufferType::TARGET);
	outputBuffer[1] = new TextureBuffer(newOutput2, BufferType::TARGET);

	raysBuffer = new Buffer<float>(m_Width * m_Height * 96 / 4);
	sRaysBuffer = new Buffer<float>(m_Width * m_Height * 96 / 4);
	colorBuffer = new Buffer<glm::vec4>(m_Width * m_Height);

	setArguments();
	Reset();
}

void WFTracer::setupCamera() { m_Camera->updateGPUBuffer(); }

void WFTracer::setupObjects()
{
	// copy initial BVH tree to GPU
	primitiveIndicesBuffer = new Buffer<unsigned int>(m_BVHTree->m_PrimitiveIndices);
	primitiveIndicesBuffer->copyToDevice();

	MBVHNodeBuffer = new Buffer<bvh::MBVHNode>(m_MBVHTree->m_Tree);
	MBVHNodeBuffer->copyToDevice();

	verticesBuffer = new Buffer(m_TList->m_Vertices);
	normalBuffer = new Buffer(m_TList->m_Normals);
	texCoordBuffer = new Buffer(m_TList->m_TexCoords);
	cnBuffer = new Buffer(m_TList->m_CenterNormals);
	indicesBuffer = new Buffer(m_TList->m_Indices);

	if (m_TList->m_LightIndices.empty())
		lightIndicesBuffer = new Buffer<unsigned int>(1);
	else
		lightIndicesBuffer = new Buffer(m_TList->m_LightIndices);

	matIndicesBuffer = new Buffer(m_TList->m_MaterialIdxs);

	verticesBuffer->copyToDevice(false);
	normalBuffer->copyToDevice(false);
	texCoordBuffer->copyToDevice(false);
	cnBuffer->copyToDevice(false);
	indicesBuffer->copyToDevice(false);
	lightIndicesBuffer->copyToDevice(false);
	matIndicesBuffer->copyToDevice(false);
}

void WFTracer::setupMaterials()
{
	materialBuffer = new Buffer(MaterialManager::GetInstance()->GetMaterials());
	materialBuffer->copyToDevice();

	microfacetBuffer = new Buffer(MaterialManager::GetInstance()->GetMicrofacets());
	microfacetBuffer->copyToDevice();
}

void WFTracer::setupTextures()
{
	if (!MaterialManager::GetInstance()->GetTextures().empty())
	{
		std::vector<TextureInfo> textureInfos{};
		std::vector<vec4> textureColors;
		for (auto *tex : MaterialManager::GetInstance()->GetTextures())
		{
			const int offset = int(textureColors.size());
			const auto texSize = tex->getWidth() * tex->getHeight();
			for (int i = 0; i < texSize; i++)
			{
				const vec4 color = tex->GetTextureBuffer()[i];
				textureColors.push_back(color);
			}
			textureInfos.emplace_back(tex->getWidth(), tex->getHeight(), offset);
		}

		textureBuffer = new Buffer(textureColors);
		textureBuffer->copyToDevice(false);
		textureInfoBuffer = new Buffer(textureInfos);
		textureInfoBuffer->copyToDevice(false);
	}
	else
	{
		textureBuffer = new Buffer<glm::vec4>(1);
		textureBuffer->copyToDevice(false);
		textureInfoBuffer = new Buffer<TextureInfo>(1);
		textureInfoBuffer->copyToDevice(false);
	}
}

void WFTracer::setupSkybox(Surface *skybox)
{
	m_HasSkybox = (skybox != nullptr);
	if (m_HasSkybox)
	{
		auto skyInfo = TextureInfo(skybox->getWidth(), skybox->getHeight(), 0);
		skyDomeBuffer = new Buffer(skybox->getTextureVector());
		skyDomeInfoBuffer = new Buffer(&skyInfo);
		skyDomeBuffer->copyToDevice(false);
		skyDomeInfoBuffer->copyToDevice(false);
	}
	else
	{
		skyDomeBuffer = new Buffer<glm::vec4>(1);
		skyDomeInfoBuffer = new Buffer<TextureInfo>(1);
	}
}

void WFTracer::setArguments()
{
	const auto lightCount = static_cast<int>(m_TList->m_LightIndices.size());
	CHECKCL(wGenerateRayKernel->setBuffer(0, colorBuffer));
	CHECKCL(wGenerateRayKernel->setBuffer(1, raysBuffer));
	CHECKCL(wGenerateRayKernel->setBuffer(2, sRaysBuffer));
	CHECKCL(wGenerateRayKernel->setBuffer(3, m_Camera->getGPUBuffer()));
	CHECKCL(wGenerateRayKernel->setArgument(4, 1, &m_Width));
	CHECKCL(wGenerateRayKernel->setArgument(5, 1, &m_Height));
	CHECKCL(wGenerateRayKernel->setArgument(6, 1, &m_Frame));
	CHECKCL(wIntersectKernel->setBuffer(0, raysBuffer));
	CHECKCL(wIntersectKernel->setBuffer(1, indicesBuffer));
	CHECKCL(wIntersectKernel->setBuffer(2, verticesBuffer));
	CHECKCL(wIntersectKernel->setBuffer(3, MBVHNodeBuffer));
	CHECKCL(wIntersectKernel->setBuffer(4, primitiveIndicesBuffer));
	CHECKCL(wIntersectKernel->setArgument(5, 1, &m_Width));
	CHECKCL(wIntersectKernel->setArgument(6, 1, &m_Height));
	CHECKCL(wShadeRefKernel->setBuffer(0, raysBuffer));
	CHECKCL(wShadeRefKernel->setBuffer(1, sRaysBuffer));
	CHECKCL(wShadeRefKernel->setBuffer(2, materialBuffer));
	CHECKCL(wShadeRefKernel->setBuffer(3, indicesBuffer));
	CHECKCL(wShadeRefKernel->setBuffer(4, lightIndicesBuffer));
	CHECKCL(wShadeRefKernel->setBuffer(5, verticesBuffer));
	CHECKCL(wShadeRefKernel->setBuffer(6, cnBuffer));
	CHECKCL(wShadeRefKernel->setBuffer(7, normalBuffer));
	CHECKCL(wShadeRefKernel->setBuffer(8, texCoordBuffer));
	CHECKCL(wShadeRefKernel->setBuffer(9, primitiveIndicesBuffer));
	CHECKCL(wShadeRefKernel->setBuffer(10, colorBuffer));
	CHECKCL(wShadeRefKernel->setBuffer(11, textureBuffer));
	CHECKCL(wShadeRefKernel->setBuffer(12, textureInfoBuffer));
	CHECKCL(wShadeRefKernel->setBuffer(13, skyDomeBuffer));
	CHECKCL(wShadeRefKernel->setBuffer(14, skyDomeInfoBuffer));
	CHECKCL(wShadeRefKernel->setBuffer(15, microfacetBuffer));
	CHECKCL(wShadeRefKernel->setBuffer(16, matIndicesBuffer));
	CHECKCL(wShadeRefKernel->setArgument(17, 1, &m_HasSkybox));
	CHECKCL(wShadeRefKernel->setArgument(18, 1, &lightCount));
	CHECKCL(wShadeRefKernel->setArgument(19, 1, &m_Width));
	CHECKCL(wShadeRefKernel->setArgument(20, 1, &m_Height));
	CHECKCL(wShadeRefKernel->setArgument(21, 1, &m_Frame));
	CHECKCL(wShadeNeeKernel->setBuffer(0, raysBuffer));
	CHECKCL(wShadeNeeKernel->setBuffer(1, sRaysBuffer));
	CHECKCL(wShadeNeeKernel->setBuffer(2, materialBuffer));
	CHECKCL(wShadeNeeKernel->setBuffer(3, indicesBuffer));
	CHECKCL(wShadeNeeKernel->setBuffer(4, lightIndicesBuffer));
	CHECKCL(wShadeNeeKernel->setBuffer(5, verticesBuffer));
	CHECKCL(wShadeNeeKernel->setBuffer(6, cnBuffer));
	CHECKCL(wShadeNeeKernel->setBuffer(7, normalBuffer));
	CHECKCL(wShadeNeeKernel->setBuffer(8, texCoordBuffer));
	CHECKCL(wShadeNeeKernel->setBuffer(9, primitiveIndicesBuffer));
	CHECKCL(wShadeNeeKernel->setBuffer(10, colorBuffer));
	CHECKCL(wShadeNeeKernel->setBuffer(11, textureBuffer));
	CHECKCL(wShadeNeeKernel->setBuffer(12, textureInfoBuffer));
	CHECKCL(wShadeNeeKernel->setBuffer(13, skyDomeBuffer));
	CHECKCL(wShadeNeeKernel->setBuffer(14, skyDomeInfoBuffer));
	CHECKCL(wShadeNeeKernel->setBuffer(15, microfacetBuffer));
	CHECKCL(wShadeNeeKernel->setBuffer(16, matIndicesBuffer));
	CHECKCL(wShadeNeeKernel->setArgument(17, 1, &m_HasSkybox));
	CHECKCL(wShadeNeeKernel->setArgument(18, 1, &lightCount));
	CHECKCL(wShadeNeeKernel->setArgument(19, 1, &m_Width));
	CHECKCL(wShadeNeeKernel->setArgument(20, 1, &m_Height));
	CHECKCL(wShadeNeeKernel->setArgument(21, 1, &m_Frame));
	CHECKCL(wShadeMisKernel->setBuffer(0, raysBuffer));
	CHECKCL(wShadeMisKernel->setBuffer(1, sRaysBuffer));
	CHECKCL(wShadeMisKernel->setBuffer(2, materialBuffer));
	CHECKCL(wShadeMisKernel->setBuffer(3, indicesBuffer));
	CHECKCL(wShadeMisKernel->setBuffer(4, lightIndicesBuffer));
	CHECKCL(wShadeMisKernel->setBuffer(5, verticesBuffer));
	CHECKCL(wShadeMisKernel->setBuffer(6, cnBuffer));
	CHECKCL(wShadeMisKernel->setBuffer(7, normalBuffer));
	CHECKCL(wShadeMisKernel->setBuffer(8, texCoordBuffer));
	CHECKCL(wShadeMisKernel->setBuffer(9, primitiveIndicesBuffer));
	CHECKCL(wShadeMisKernel->setBuffer(10, colorBuffer));
	CHECKCL(wShadeMisKernel->setBuffer(11, textureBuffer));
	CHECKCL(wShadeMisKernel->setBuffer(12, textureInfoBuffer));
	CHECKCL(wShadeMisKernel->setBuffer(13, skyDomeBuffer));
	CHECKCL(wShadeMisKernel->setBuffer(14, skyDomeInfoBuffer));
	CHECKCL(wShadeMisKernel->setBuffer(15, microfacetBuffer));
	CHECKCL(wShadeMisKernel->setBuffer(16, matIndicesBuffer));
	CHECKCL(wShadeMisKernel->setArgument(17, 1, &m_HasSkybox));
	CHECKCL(wShadeMisKernel->setArgument(18, 1, &lightCount));
	CHECKCL(wShadeMisKernel->setArgument(19, 1, &m_Width));
	CHECKCL(wShadeMisKernel->setArgument(20, 1, &m_Height));
	CHECKCL(wShadeMisKernel->setArgument(21, 1, &m_Frame));
	CHECKCL(wShadeRayTracer->setBuffer(0, raysBuffer));
	CHECKCL(wShadeRayTracer->setBuffer(1, sRaysBuffer));
	CHECKCL(wShadeRayTracer->setBuffer(2, materialBuffer));
	CHECKCL(wShadeRayTracer->setBuffer(3, indicesBuffer));
	CHECKCL(wShadeRayTracer->setBuffer(4, lightIndicesBuffer));
	CHECKCL(wShadeRayTracer->setBuffer(5, verticesBuffer));
	CHECKCL(wShadeRayTracer->setBuffer(6, cnBuffer));
	CHECKCL(wShadeRayTracer->setBuffer(7, normalBuffer));
	CHECKCL(wShadeRayTracer->setBuffer(8, texCoordBuffer));
	CHECKCL(wShadeRayTracer->setBuffer(9, primitiveIndicesBuffer));
	CHECKCL(wShadeRayTracer->setBuffer(10, colorBuffer));
	CHECKCL(wShadeRayTracer->setBuffer(11, textureBuffer));
	CHECKCL(wShadeRayTracer->setBuffer(12, textureInfoBuffer));
	CHECKCL(wShadeRayTracer->setBuffer(13, skyDomeBuffer));
	CHECKCL(wShadeRayTracer->setBuffer(14, skyDomeInfoBuffer));
	CHECKCL(wShadeRayTracer->setBuffer(15, microfacetBuffer));
	CHECKCL(wShadeRayTracer->setBuffer(16, matIndicesBuffer));
	CHECKCL(wShadeRayTracer->setArgument(17, 1, &m_HasSkybox));
	CHECKCL(wShadeRayTracer->setArgument(18, 1, &lightCount));
	CHECKCL(wShadeRayTracer->setArgument(19, 1, &m_Width));
	CHECKCL(wShadeRayTracer->setArgument(20, 1, &m_Height));
	CHECKCL(wShadeRayTracer->setArgument(21, 1, &m_Frame));
	CHECKCL(wConnectKernel->setBuffer(0, sRaysBuffer));
	CHECKCL(wConnectKernel->setBuffer(1, indicesBuffer));
	CHECKCL(wConnectKernel->setBuffer(2, verticesBuffer));
	CHECKCL(wConnectKernel->setBuffer(3, primitiveIndicesBuffer));
	CHECKCL(wConnectKernel->setBuffer(4, colorBuffer));
	CHECKCL(wConnectKernel->setBuffer(5, MBVHNodeBuffer));
	CHECKCL(wConnectKernel->setArgument(6, 1, &m_Width));
	CHECKCL(wConnectKernel->setArgument(7, 1, &m_Height));
	CHECKCL(wDrawKernel->setBuffer(0, outputBuffer[0]));
	CHECKCL(wDrawKernel->setBuffer(1, colorBuffer));
	CHECKCL(wDrawKernel->setArgument(2, 1, &m_Width));
	CHECKCL(wDrawKernel->setArgument(3, 1, &m_Height));
}

} // namespace core
