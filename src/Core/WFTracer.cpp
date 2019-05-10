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
	m_Modes = {"Ref", "Nee", "Mis"};
	Kernel::InitCL();
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
	wConnectKernel = new Kernel("cl-src/program.cl", "connect", workSize);
	wDrawKernel = new Kernel("cl-src/program.cl", "draw", workSize);

	Resize(0, 0, t1, t2);
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

	delete outputBuffer[0];
	delete outputBuffer[1];
	delete primitiveIndicesBuffer;
	delete MBVHNodeBuffer;
	delete colorBuffer;
	delete textureBuffer;
	delete textureInfoBuffer;
	delete skyDomeBuffer;
	delete skyDomeInfoBuffer;
	delete raysBuffer;
	delete sRaysBuffer;
	delete materialBuffer;
	delete microfacetBuffer;
	delete matIndicesBuffer;
	delete verticesBuffer;
	delete normalBuffer;
	delete texCoordBuffer;
	delete cnBuffer;
	delete indicesBuffer;
	delete lightIndicesBuffer;
	delete wGenerateRayKernel;
	delete wIntersectKernel;
	delete wShadeRefKernel;
	delete wShadeNeeKernel;
	delete wShadeMisKernel;
	delete wConnectKernel;
	delete wDrawKernel;
}

void WFTracer::Render(Surface *)
{
	if (m_Frame <= 0)
		m_Frame = 1;

	wGenerateRayKernel->setArgument(0, outputBuffer[tIdx]);
	wDrawKernel->setArgument(0, outputBuffer[tIdx]);

	wGenerateRayKernel->setArgument(7, m_Frame);
	wShadeRefKernel->setArgument(21, m_Frame);
	wShadeNeeKernel->setArgument(21, m_Frame);
	wShadeMisKernel->setArgument(21, m_Frame);
	wDrawKernel->setArgument(4, m_Frame);

	wGenerateRayKernel->run(outputBuffer[tIdx]);
	wIntersectKernel->run();

	Kernel::syncQueue();
	auto result = m_Camera->updateGPUBufferAsync();

	switch (m_Mode)
	{
	case (1):
		wShadeNeeKernel->run();
		Kernel::syncQueue();
		wConnectKernel->run();
		break;
	case (2):
		wShadeMisKernel->run();
		Kernel::syncQueue();
		wConnectKernel->run();
		break;
	case (0):
	default:
		wShadeRefKernel->run();
		break;
	}

	wDrawKernel->run(outputBuffer[tIdx]);
	tIdx = 1 - tIdx;
	m_Samples++;
	m_Frame++;

	result.get();
}

void WFTracer::Reset()
{
	colorBuffer->clear();
	outputBuffer[0]->clear();
	outputBuffer[1]->clear();
	m_Samples = 0;
	m_Frame = 1;
}

void WFTracer::Resize(int width, int height, gl::Texture *newOutput1, gl::Texture *newOutput2)
{
	outputTexture[0] = newOutput1;
	outputTexture[1] = newOutput2;
	m_Width = width;
	m_Height = height;
	setupCamera();

	auto workSizeX = m_Width + (m_Width % 8);
	auto workSizeY = m_Height + (m_Height % 8);

	wGenerateRayKernel->SetWorkSize(workSizeX, workSizeY, 1);
	wIntersectKernel->SetWorkSize(workSizeX, workSizeY, 1);
	wShadeRefKernel->SetWorkSize(workSizeX, workSizeY, 1);
	wShadeNeeKernel->SetWorkSize(workSizeX, workSizeY, 1);
	wShadeMisKernel->SetWorkSize(workSizeX, workSizeY, 1);
	wConnectKernel->SetWorkSize(workSizeX, workSizeY, 1);
	wDrawKernel->SetWorkSize(workSizeX, workSizeY, 1);

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

	verticesBuffer->copyToDevice();
	normalBuffer->copyToDevice();
	texCoordBuffer->copyToDevice();
	cnBuffer->copyToDevice();
	indicesBuffer->copyToDevice();
	lightIndicesBuffer->copyToDevice();
	matIndicesBuffer->copyToDevice();
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
	if (!m_TList->m_Textures.empty())
	{
		std::vector<TextureInfo> textureInfos{};
		std::vector<vec4> textureColors;
		for (auto *tex : m_TList->m_Textures)
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
		textureBuffer->copyToDevice();
		textureInfoBuffer = new Buffer(textureInfos);
		textureInfoBuffer->copyToDevice();
	}
	else
	{
		textureBuffer = new Buffer<glm::vec4>(1);
		textureBuffer->copyToDevice();
		textureInfoBuffer = new Buffer<TextureInfo>(1);
		textureInfoBuffer->copyToDevice();
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
		skyDomeBuffer->copyToDevice();
		skyDomeInfoBuffer->copyToDevice();
	}
	else
	{
		auto dummy = glm::vec4(1.0f);
		skyDomeBuffer = new Buffer(&dummy);
		auto dummy1 = TextureInfo(0, 0, 0);
		skyDomeInfoBuffer = new Buffer(&dummy1);
	}
}

void WFTracer::setArguments()
{
	wGenerateRayKernel->setArgument(0, outputBuffer[0]);
	wGenerateRayKernel->setArgument(1, colorBuffer);
	wGenerateRayKernel->setArgument(2, raysBuffer);
	wGenerateRayKernel->setArgument(3, sRaysBuffer);
	wGenerateRayKernel->setArgument(4, m_Camera->getGPUBuffer());
	wGenerateRayKernel->setArgument(5, m_Width);
	wGenerateRayKernel->setArgument(6, m_Height);
	wGenerateRayKernel->setArgument(7, m_Frame);

	wIntersectKernel->setArgument(0, raysBuffer);
	wIntersectKernel->setArgument(1, indicesBuffer);
	wIntersectKernel->setArgument(2, verticesBuffer);
	wIntersectKernel->setArgument(3, MBVHNodeBuffer);
	wIntersectKernel->setArgument(4, primitiveIndicesBuffer);
	wIntersectKernel->setArgument(5, m_Width);
	wIntersectKernel->setArgument(6, m_Height);

	wShadeRefKernel->setArgument(0, raysBuffer);
	wShadeRefKernel->setArgument(1, sRaysBuffer);
	wShadeRefKernel->setArgument(2, materialBuffer);
	wShadeRefKernel->setArgument(3, indicesBuffer);
	wShadeRefKernel->setArgument(4, lightIndicesBuffer);
	wShadeRefKernel->setArgument(5, verticesBuffer);
	wShadeRefKernel->setArgument(6, cnBuffer);
	wShadeRefKernel->setArgument(7, normalBuffer);
	wShadeRefKernel->setArgument(8, texCoordBuffer);
	wShadeRefKernel->setArgument(9, primitiveIndicesBuffer);
	wShadeRefKernel->setArgument(10, colorBuffer);
	wShadeRefKernel->setArgument(11, textureBuffer);
	wShadeRefKernel->setArgument(12, textureInfoBuffer);
	wShadeRefKernel->setArgument(13, skyDomeBuffer);
	wShadeRefKernel->setArgument(14, skyDomeInfoBuffer);
	wShadeRefKernel->setArgument(15, microfacetBuffer);
	wShadeRefKernel->setArgument(16, matIndicesBuffer);
	wShadeRefKernel->setArgument(17, m_HasSkybox ? 1 : 0);
	wShadeRefKernel->setArgument(18, int(m_TList->m_LightIndices.size()));
	wShadeRefKernel->setArgument(19, m_Width);
	wShadeRefKernel->setArgument(20, m_Height);
	wShadeRefKernel->setArgument(21, m_Frame);

	wShadeNeeKernel->setArgument(0, raysBuffer);
	wShadeNeeKernel->setArgument(1, sRaysBuffer);
	wShadeNeeKernel->setArgument(2, materialBuffer);
	wShadeNeeKernel->setArgument(3, indicesBuffer);
	wShadeNeeKernel->setArgument(4, lightIndicesBuffer);
	wShadeNeeKernel->setArgument(5, verticesBuffer);
	wShadeNeeKernel->setArgument(6, cnBuffer);
	wShadeNeeKernel->setArgument(7, normalBuffer);
	wShadeNeeKernel->setArgument(8, texCoordBuffer);
	wShadeNeeKernel->setArgument(9, primitiveIndicesBuffer);
	wShadeNeeKernel->setArgument(10, colorBuffer);
	wShadeNeeKernel->setArgument(11, textureBuffer);
	wShadeNeeKernel->setArgument(12, textureInfoBuffer);
	wShadeNeeKernel->setArgument(13, skyDomeBuffer);
	wShadeNeeKernel->setArgument(14, skyDomeInfoBuffer);
	wShadeNeeKernel->setArgument(15, microfacetBuffer);
	wShadeNeeKernel->setArgument(16, matIndicesBuffer);
	wShadeNeeKernel->setArgument(17, m_HasSkybox ? 1 : 0);
	wShadeNeeKernel->setArgument(18, int(m_TList->m_LightIndices.size()));
	wShadeNeeKernel->setArgument(19, m_Width);
	wShadeNeeKernel->setArgument(20, m_Height);
	wShadeNeeKernel->setArgument(21, m_Frame);

	wShadeMisKernel->setArgument(0, raysBuffer);
	wShadeMisKernel->setArgument(1, sRaysBuffer);
	wShadeMisKernel->setArgument(2, materialBuffer);
	wShadeMisKernel->setArgument(3, indicesBuffer);
	wShadeMisKernel->setArgument(4, lightIndicesBuffer);
	wShadeMisKernel->setArgument(5, verticesBuffer);
	wShadeMisKernel->setArgument(6, cnBuffer);
	wShadeMisKernel->setArgument(7, normalBuffer);
	wShadeMisKernel->setArgument(8, texCoordBuffer);
	wShadeMisKernel->setArgument(9, primitiveIndicesBuffer);
	wShadeMisKernel->setArgument(10, colorBuffer);
	wShadeMisKernel->setArgument(11, textureBuffer);
	wShadeMisKernel->setArgument(12, textureInfoBuffer);
	wShadeMisKernel->setArgument(13, skyDomeBuffer);
	wShadeMisKernel->setArgument(14, skyDomeInfoBuffer);
	wShadeMisKernel->setArgument(15, microfacetBuffer);
	wShadeMisKernel->setArgument(16, matIndicesBuffer);
	wShadeMisKernel->setArgument(17, m_HasSkybox ? 1 : 0);
	wShadeMisKernel->setArgument(18, int(m_TList->m_LightIndices.size()));
	wShadeMisKernel->setArgument(19, m_Width);
	wShadeMisKernel->setArgument(20, m_Height);
	wShadeMisKernel->setArgument(21, m_Frame);

	wConnectKernel->setArgument(0, sRaysBuffer);
	wConnectKernel->setArgument(1, indicesBuffer);
	wConnectKernel->setArgument(2, verticesBuffer);
	wConnectKernel->setArgument(3, primitiveIndicesBuffer);
	wConnectKernel->setArgument(4, colorBuffer);
	wConnectKernel->setArgument(5, MBVHNodeBuffer);
	wConnectKernel->setArgument(6, m_Width);
	wConnectKernel->setArgument(7, m_Height);

	wDrawKernel->setArgument(0, outputBuffer[0]);
	wDrawKernel->setArgument(1, colorBuffer);
	wDrawKernel->setArgument(2, m_Width);
	wDrawKernel->setArgument(3, m_Height);
}

} // namespace core
