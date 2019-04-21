#include "WFTracer.h"

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
	}
};

struct GpuCamera
{
	glm::vec4 origin;		 // 16
	glm::vec4 viewDirection; // 32

	float viewDistance; // 36
	float invWidth;		// 40
	float invHeight;	// 44
	float aspectRatio;  // 48
};

WFTracer::WFTracer(TriangleList *tList, gl::Texture *t1, gl::Texture *t2, Camera *camera, Surface *skybox,
				   ctpl::ThreadPool *pool)
	: m_Camera(camera), m_TList(tList)
{
	modes = {"Reference"};
	Kernel::InitCL();
	m_BVHTree = new bvh::StaticBVHTree(tList->m_AABBs, bvh::BVHType::SAH_BINNING, pool);
	m_BVHTree->ConstructBVH();
	m_MBVHTree = new bvh::MBVHTree(m_BVHTree);

	outputTexture[0] = t1;
	outputTexture[1] = t2;

	this->width = t1->GetWidth();
	this->height = t1->GetHeight();

	auto workSize = std::tuple<size_t, size_t, size_t>(width, height, 1);
	auto localSize = std::tuple<size_t, size_t, size_t>(8, 8, 1);

	wGenerateRayKernel = new Kernel("cl-src/program.cl", "generate", workSize, localSize);
	wIntersectKernel = new Kernel("cl-src/program.cl", "intersect", workSize, localSize);
	wShadeKernel = new Kernel("cl-src/program.cl", "shade", workSize, localSize);
	wDrawKernel = new Kernel("cl-src/program.cl", "draw", workSize, localSize);

	Resize(t1);
	setupObjects();
	setupMaterials();
	setupTextures();
	setupNEEData();
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
	delete skyDome;
	delete skyDomeInfo;
	delete raysBuffer;
	delete cameraBuffer;
	delete materialBuffer;
	delete microfacetBuffer;
	delete matIndicesBuffer;
	delete verticesBuffer;
	delete normalBuffer;
	delete texCoordBuffer;
	delete cnBuffer;
	delete indicesBuffer;

	delete lightIndices;
	delete lightLotteryTickets;
	delete wGenerateRayKernel;
	delete wIntersectKernel;
	delete wShadeKernel;
	delete wDrawKernel;
}

void WFTracer::Render(Surface *output)
{
	frame++;

	wGenerateRayKernel->SetArgument(8, frame);
	wShadeKernel->SetArgument(18, frame);

	wGenerateRayKernel->Run(outputBuffer);
	wIntersectKernel->Run();
	wShadeKernel->Run();
	wDrawKernel->Run(outputBuffer);
	samples++;

	if (m_Camera->isDirty)
	{
		setupCamera();
		// calculate on CPU to make sure we only calculate it once
		const glm::vec3 u = normalize(cross(m_Camera->GetViewDirection(), vec3(0, 1, 0)));
		const glm::vec3 v = normalize(cross(u, m_Camera->GetViewDirection()));
		const glm::vec3 horizontal = u * m_Camera->GetFOVDistance() * m_Camera->GetAspectRatio();
		const glm::vec3 vertical = v * m_Camera->GetFOVDistance();
		wGenerateRayKernel->SetArgument(4, vec4(horizontal, 1.0f));
		wGenerateRayKernel->SetArgument(5, vec4(vertical, 1.0f));
		m_Camera->isDirty = false;
	}
}

void WFTracer::Reset()
{
	samples = 0;
	frame = 0;
}

void WFTracer::Resize(gl::Texture *newOutput)
{
	outputTexture[0] = newOutput;

	width = newOutput->GetWidth();
	height = newOutput->GetHeight();

	delete outputBuffer;
	delete raysBuffer;
	delete colorBuffer;
	delete cameraBuffer;

	outputBuffer = nullptr;
	raysBuffer = nullptr;
	colorBuffer = nullptr;
	cameraBuffer = nullptr;

	setupCamera();

	// set up textures
	outputBuffer = new Buffer(newOutput, BufferType::TARGET);

	// create buffer to store primary ray
	raysBuffer = new Buffer(width * height * 128);

	// create buffer to store colors for accumulation
	colorBuffer = new Buffer(width * height * sizeof(glm::vec4));
}

void WFTracer::setupCamera()
{
	// create a buffer for the camera
	if (cameraBuffer == nullptr)
		cameraBuffer = new Buffer(sizeof(GpuCamera));

	auto *cam = cameraBuffer->GetHostPtr<GpuCamera>();
	cam->origin = vec4(m_Camera->GetPosition(), 0.0f);
	cam->viewDirection = vec4(m_Camera->GetViewDirection(), 0.0f);
	cam->viewDistance = m_Camera->GetFOVDistance();
	cam->invWidth = m_Camera->GetInvWidth();
	cam->invHeight = m_Camera->GetInvHeight();
	cam->aspectRatio = m_Camera->GetAspectRatio();

	cameraBuffer->CopyToDevice();
}

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
	lightIndices = new Buffer(m_TList->m_LightIndices.size() * sizeof(int) * 4, m_TList->m_LightIndices.data());
	matIndicesBuffer = new Buffer(m_TList->m_MaterialIdxs.size() * sizeof(int), m_TList->m_MaterialIdxs.data());

	verticesBuffer->CopyToDevice();
	normalBuffer->CopyToDevice();
	texCoordBuffer->CopyToDevice();
	cnBuffer->CopyToDevice();
	indicesBuffer->CopyToDevice();
	lightIndices->CopyToDevice();
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

void WFTracer::setupNEEData() {}

void WFTracer::setupTextures()
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

		std::vector<vec4> textureColors(vec4Offset);
		for (unsigned int i = 0; i < textureInfos.size(); i++)
		{
			const int offset = textureInfos[i].offset;

			auto tex = MaterialManager::GetInstance()->GetTextures()[i];
			memcpy(&textureColors.data()[offset], tex->GetTextureBuffer(),
				   tex->GetWidth() * tex->GetHeight() * sizeof(vec4));
		}

		textureBuffer = new Buffer(vec4Offset * sizeof(vec4), textureColors.data());
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
	delete skyDome;
	delete skyDomeInfo;

	m_HasSkybox = (skybox != nullptr);
	if (m_HasSkybox)
	{
		auto skyInfo = TextureInfo(skybox->GetWidth(), skybox->GetHeight(), 0);
		skyDome = new Buffer(skybox->GetWidth() * skybox->GetHeight() * sizeof(glm::vec4), skybox->GetTextureBuffer());
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

void WFTracer::setArguments()
{
	wGenerateRayKernel->SetArgument(0, outputBuffer);
	wGenerateRayKernel->SetArgument(1, colorBuffer);
	wGenerateRayKernel->SetArgument(2, raysBuffer);
	wGenerateRayKernel->SetArgument(3, cameraBuffer);
	const glm::vec3 u = normalize(cross(m_Camera->GetViewDirection(), vec3(0, 1, 0)));
	const glm::vec3 v = normalize(cross(u, m_Camera->GetViewDirection()));
	const glm::vec3 horizontal = u * m_Camera->GetFOVDistance() * m_Camera->GetAspectRatio();
	const glm::vec3 vertical = v * m_Camera->GetFOVDistance();
	wGenerateRayKernel->SetArgument(4, vec4(horizontal, 1.0f));
	wGenerateRayKernel->SetArgument(5, vec4(vertical, 1.0f));
	wGenerateRayKernel->SetArgument(6, width);
	wGenerateRayKernel->SetArgument(7, height);
	wGenerateRayKernel->SetArgument(8, frame);

	wIntersectKernel->SetArgument(0, raysBuffer);
	wIntersectKernel->SetArgument(1, indicesBuffer);
	wIntersectKernel->SetArgument(2, verticesBuffer);
	wIntersectKernel->SetArgument(3, MBVHNodeBuffer);
	wIntersectKernel->SetArgument(4, primitiveIndicesBuffer);
	wIntersectKernel->SetArgument(5, width);
	wIntersectKernel->SetArgument(6, height);

	wShadeKernel->SetArgument(0, raysBuffer);
	wShadeKernel->SetArgument(1, materialBuffer);
	wShadeKernel->SetArgument(2, indicesBuffer);
	wShadeKernel->SetArgument(3, verticesBuffer);
	wShadeKernel->SetArgument(4, cnBuffer);
	wShadeKernel->SetArgument(5, normalBuffer);
	wShadeKernel->SetArgument(6, texCoordBuffer);
	wShadeKernel->SetArgument(7, primitiveIndicesBuffer);
	wShadeKernel->SetArgument(8, colorBuffer);
	wShadeKernel->SetArgument(9, textureBuffer);
	wShadeKernel->SetArgument(10, textureInfoBuffer);
	wShadeKernel->SetArgument(11, skyDome);
	wShadeKernel->SetArgument(12, skyDomeInfo);
	wShadeKernel->SetArgument(13, microfacetBuffer);
	wShadeKernel->SetArgument(14, matIndicesBuffer);
	wShadeKernel->SetArgument(15, m_HasSkybox ? 1 : 0);
	wShadeKernel->SetArgument(16, width);
	wShadeKernel->SetArgument(17, height);
	wShadeKernel->SetArgument(18, frame);

	wDrawKernel->SetArgument(0, outputBuffer);
	wDrawKernel->SetArgument(1, colorBuffer);
	wDrawKernel->SetArgument(2, samples);
	wDrawKernel->SetArgument(3, width);
	wDrawKernel->SetArgument(4, height);
}

} // namespace core
