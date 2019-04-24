#include "WFTracer.h"

#include <future>

using namespace cl;
using namespace gl;
using namespace core;

namespace core
{

struct TextureInfo
{
	union {
		glm::ivec4 data{};
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
		this->dummy = 0;
	}
};

WFTracer::WFTracer(TriangleList *tList, gl::Texture *t1, gl::Texture *t2, Camera *camera, Surface *skybox,
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

	Resize(t1);
	setupObjects();
	setupMaterials();
	setupTextures();
	setupSkybox(skybox);
	setArguments();
	colorBuffer->Clear();
}

WFTracer::~WFTracer()
{
	delete m_BVHTree;
	delete m_MBVHTree;

	delete outputBuffer;
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

void WFTracer::Render(Surface *output)
{
	m_Frame++;

	wGenerateRayKernel->SetArgument(7, m_Frame);
	wShadeRefKernel->SetArgument(21, m_Frame);
	wShadeNeeKernel->SetArgument(21, m_Frame);
	wShadeMisKernel->SetArgument(21, m_Frame);

	wGenerateRayKernel->Run(outputBuffer);
	wIntersectKernel->Run();

	auto result = m_Camera->updateGPUBufferAsync();

	switch (m_Mode)
	{
	case (1):
		wShadeNeeKernel->Run();
		Kernel::SyncQueue();
		wConnectKernel->Run();
		break;
	case (2):
		wShadeMisKernel->Run();
		Kernel::SyncQueue();
		wConnectKernel->Run();
		break;
	case (0):
	default:
		wShadeRefKernel->Run();
		break;
	}

	wDrawKernel->Run(outputBuffer);
	m_Samples++;
	result.get();
}

void WFTracer::Reset()
{
	m_Samples = 0;
	m_Frame = 0;
}

void WFTracer::Resize(gl::Texture *newOutput)
{
	outputTexture[0] = newOutput;
	m_Width = newOutput->GetWidth();
	m_Height = newOutput->GetHeight();
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

	outputBuffer = new Buffer(newOutput, BufferType::TARGET);
	raysBuffer = new Buffer(m_Width * m_Height * 96);
	sRaysBuffer = new Buffer(m_Width * m_Height * 64);
	colorBuffer = new Buffer(m_Width * m_Height * sizeof(glm::vec4));

	setArguments();
	Reset();
}

void WFTracer::setupCamera() { m_Camera->updateGPUBuffer(); }

void WFTracer::setupObjects()
{
	// copy initial BVH tree to GPU
	primitiveIndicesBuffer = new Buffer(static_cast<unsigned int>(m_BVHTree->m_PrimitiveCount) * sizeof(unsigned int),
										m_BVHTree->m_PrimitiveIndices.data());
	primitiveIndicesBuffer->CopyToDevice();
	MBVHNodeBuffer = new Buffer((m_MBVHTree->m_FinalPtr + 1) * sizeof(bvh::MBVHNode), m_MBVHTree->m_Tree.data());
	MBVHNodeBuffer->CopyToDevice();

	verticesBuffer = new Buffer(m_TList->m_Vertices.size() * sizeof(vec4), m_TList->m_Vertices.data());
	normalBuffer = new Buffer(m_TList->m_Normals.size() * sizeof(vec4), m_TList->m_Normals.data());
	texCoordBuffer = new Buffer(m_TList->m_TexCoords.size() * sizeof(vec2), m_TList->m_TexCoords.data());
	cnBuffer = new Buffer(m_TList->m_CenterNormals.size() * sizeof(vec4), m_TList->m_CenterNormals.data());
	indicesBuffer = new Buffer(m_TList->m_Indices.size() * sizeof(int) * 4, m_TList->m_Indices.data());

	if (m_TList->m_LightIndices.empty())
		lightIndicesBuffer = new Buffer(sizeof(int) * 4);
	else
		lightIndicesBuffer =
			new Buffer(m_TList->m_LightIndices.size() * sizeof(int) * 4, m_TList->m_LightIndices.data());

	matIndicesBuffer = new Buffer(m_TList->m_MaterialIdxs.size() * sizeof(int), m_TList->m_MaterialIdxs.data());

	verticesBuffer->CopyToDevice();
	normalBuffer->CopyToDevice();
	texCoordBuffer->CopyToDevice();
	cnBuffer->CopyToDevice();
	indicesBuffer->CopyToDevice();
	lightIndicesBuffer->CopyToDevice();
	matIndicesBuffer->CopyToDevice();
}

void WFTracer::setupMaterials()
{
	materialBuffer = new Buffer(MaterialManager::GetInstance()->GetMaterials().size() * sizeof(Material),
								MaterialManager::GetInstance()->GetMaterials().data());
	materialBuffer->CopyToDevice();

	microfacetBuffer = new Buffer(static_cast<unsigned int>(MaterialManager::GetInstance()->GetMicrofacets().size()) *
									  sizeof(Microfacet),
								  (void *)MaterialManager::GetInstance()->GetMicrofacets().data());
	microfacetBuffer->CopyToDevice();
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
			const auto texSize = tex->GetWidth() * tex->GetHeight();
			for (int i = 0; i < texSize; i++)
			{
				const vec4 color = tex->GetTextureBuffer()[i];
				textureColors.push_back(color);
			}
			textureInfos.emplace_back(tex->GetWidth(), tex->GetHeight(), offset);
		}

		textureBuffer = new Buffer(textureColors.size() * sizeof(vec4), textureColors.data());
		textureBuffer->CopyToDevice();
		textureInfoBuffer = new Buffer(textureInfos.size() * sizeof(TextureInfo), textureInfos.data());
		textureInfoBuffer->CopyToDevice();
	}
	else
	{
		textureBuffer = new Buffer(1 * sizeof(vec4));
		textureBuffer->CopyToDevice();
		textureInfoBuffer = new Buffer(sizeof(TextureInfo));
		textureInfoBuffer->CopyToDevice();
	}
}

void WFTracer::setupSkybox(Surface *skybox)
{
	m_HasSkybox = (skybox != nullptr);
	if (m_HasSkybox)
	{
		auto skyInfo = TextureInfo(skybox->GetWidth(), skybox->GetHeight(), 0);
		skyDomeBuffer =
			new Buffer(skybox->GetWidth() * skybox->GetHeight() * sizeof(glm::vec4), skybox->GetTextureBuffer());
		skyDomeInfoBuffer = new Buffer(sizeof(TextureInfo), &skyInfo);
		skyDomeBuffer->CopyToDevice();
		skyDomeInfoBuffer->CopyToDevice();
	}
	else
	{
		skyDomeBuffer = new Buffer(4);
		skyDomeInfoBuffer = new Buffer(4);
	}
}

void WFTracer::setArguments()
{
	wGenerateRayKernel->SetArgument(1, colorBuffer);
	wGenerateRayKernel->SetArgument(0, outputBuffer);
	wGenerateRayKernel->SetArgument(2, raysBuffer);
	wGenerateRayKernel->SetArgument(3, sRaysBuffer);
	wGenerateRayKernel->SetArgument(4, m_Camera->getGPUBuffer());
	wGenerateRayKernel->SetArgument(5, m_Width);
	wGenerateRayKernel->SetArgument(6, m_Height);
	wGenerateRayKernel->SetArgument(7, m_Frame);

	wIntersectKernel->SetArgument(0, raysBuffer);
	wIntersectKernel->SetArgument(1, indicesBuffer);
	wIntersectKernel->SetArgument(2, verticesBuffer);
	wIntersectKernel->SetArgument(3, MBVHNodeBuffer);
	wIntersectKernel->SetArgument(4, primitiveIndicesBuffer);
	wIntersectKernel->SetArgument(5, m_Width);
	wIntersectKernel->SetArgument(6, m_Height);

	wShadeRefKernel->SetArgument(0, raysBuffer);
	wShadeRefKernel->SetArgument(1, sRaysBuffer);
	wShadeRefKernel->SetArgument(2, materialBuffer);
	wShadeRefKernel->SetArgument(3, indicesBuffer);
	wShadeRefKernel->SetArgument(4, lightIndicesBuffer);
	wShadeRefKernel->SetArgument(5, verticesBuffer);
	wShadeRefKernel->SetArgument(6, cnBuffer);
	wShadeRefKernel->SetArgument(7, normalBuffer);
	wShadeRefKernel->SetArgument(8, texCoordBuffer);
	wShadeRefKernel->SetArgument(9, primitiveIndicesBuffer);
	wShadeRefKernel->SetArgument(10, colorBuffer);
	wShadeRefKernel->SetArgument(11, textureBuffer);
	wShadeRefKernel->SetArgument(12, textureInfoBuffer);
	wShadeRefKernel->SetArgument(13, skyDomeBuffer);
	wShadeRefKernel->SetArgument(14, skyDomeInfoBuffer);
	wShadeRefKernel->SetArgument(15, microfacetBuffer);
	wShadeRefKernel->SetArgument(16, matIndicesBuffer);
	wShadeRefKernel->SetArgument(17, m_HasSkybox ? 1 : 0);
	wShadeRefKernel->SetArgument(18, int(m_TList->m_LightIndices.size()));
	wShadeRefKernel->SetArgument(19, m_Width);
	wShadeRefKernel->SetArgument(20, m_Height);
	wShadeRefKernel->SetArgument(21, m_Frame);

	wShadeNeeKernel->SetArgument(0, raysBuffer);
	wShadeNeeKernel->SetArgument(1, sRaysBuffer);
	wShadeNeeKernel->SetArgument(2, materialBuffer);
	wShadeNeeKernel->SetArgument(3, indicesBuffer);
	wShadeNeeKernel->SetArgument(4, lightIndicesBuffer);
	wShadeNeeKernel->SetArgument(5, verticesBuffer);
	wShadeNeeKernel->SetArgument(6, cnBuffer);
	wShadeNeeKernel->SetArgument(7, normalBuffer);
	wShadeNeeKernel->SetArgument(8, texCoordBuffer);
	wShadeNeeKernel->SetArgument(9, primitiveIndicesBuffer);
	wShadeNeeKernel->SetArgument(10, colorBuffer);
	wShadeNeeKernel->SetArgument(11, textureBuffer);
	wShadeNeeKernel->SetArgument(12, textureInfoBuffer);
	wShadeNeeKernel->SetArgument(13, skyDomeBuffer);
	wShadeNeeKernel->SetArgument(14, skyDomeInfoBuffer);
	wShadeNeeKernel->SetArgument(15, microfacetBuffer);
	wShadeNeeKernel->SetArgument(16, matIndicesBuffer);
	wShadeNeeKernel->SetArgument(17, m_HasSkybox ? 1 : 0);
	wShadeNeeKernel->SetArgument(18, int(m_TList->m_LightIndices.size()));
	wShadeNeeKernel->SetArgument(19, m_Width);
	wShadeNeeKernel->SetArgument(20, m_Height);
	wShadeNeeKernel->SetArgument(21, m_Frame);

	wShadeMisKernel->SetArgument(0, raysBuffer);
	wShadeMisKernel->SetArgument(1, sRaysBuffer);
	wShadeMisKernel->SetArgument(2, materialBuffer);
	wShadeMisKernel->SetArgument(3, indicesBuffer);
	wShadeMisKernel->SetArgument(4, lightIndicesBuffer);
	wShadeMisKernel->SetArgument(5, verticesBuffer);
	wShadeMisKernel->SetArgument(6, cnBuffer);
	wShadeMisKernel->SetArgument(7, normalBuffer);
	wShadeMisKernel->SetArgument(8, texCoordBuffer);
	wShadeMisKernel->SetArgument(9, primitiveIndicesBuffer);
	wShadeMisKernel->SetArgument(10, colorBuffer);
	wShadeMisKernel->SetArgument(11, textureBuffer);
	wShadeMisKernel->SetArgument(12, textureInfoBuffer);
	wShadeMisKernel->SetArgument(13, skyDomeBuffer);
	wShadeMisKernel->SetArgument(14, skyDomeInfoBuffer);
	wShadeMisKernel->SetArgument(15, microfacetBuffer);
	wShadeMisKernel->SetArgument(16, matIndicesBuffer);
	wShadeMisKernel->SetArgument(17, m_HasSkybox ? 1 : 0);
	wShadeMisKernel->SetArgument(18, int(m_TList->m_LightIndices.size()));
	wShadeMisKernel->SetArgument(19, m_Width);
	wShadeMisKernel->SetArgument(20, m_Height);
	wShadeMisKernel->SetArgument(21, m_Frame);

	wConnectKernel->SetArgument(0, sRaysBuffer);
	wConnectKernel->SetArgument(1, indicesBuffer);
	wConnectKernel->SetArgument(2, verticesBuffer);
	wConnectKernel->SetArgument(3, primitiveIndicesBuffer);
	wConnectKernel->SetArgument(4, colorBuffer);
	wConnectKernel->SetArgument(5, MBVHNodeBuffer);
	wConnectKernel->SetArgument(6, m_Width);
	wConnectKernel->SetArgument(7, m_Height);

	wDrawKernel->SetArgument(0, outputBuffer);
	wDrawKernel->SetArgument(1, colorBuffer);
	wDrawKernel->SetArgument(2, m_Width);
	wDrawKernel->SetArgument(3, m_Height);
}

} // namespace core
