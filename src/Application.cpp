#include "Application.h"
#include "Materials/MaterialManager.h"
#include "imgui.h"

#include <algorithm>
#include <deque>

#define FPS_FRAMES 50
#define TEAPOT 0
#define CUBES 0

static std::deque<float> frametimes = {};
static int lastMode = 0;
static int renderMode = 0;
static float maxFrametime = 10.0f;
auto *gameObjects = new std::vector<bvh::GameObject *>();

bvh::GameObject *motherGameObject;
#if CUBES
bvh::GameObject *cubeMother1, *cubeMother2, *cubeMother3, *cubeMother4;
#endif

Application::Application(utils::Window *window, RendererType type, int width, int height, const char *scene,
						 const char *skybox)
	: m_Type(type), m_tIndex(0), m_Width(width), m_Height(height), m_Window(window)
{
	using namespace bvh;
	using namespace core;

	for (auto &key : m_KeyStatus)
		key = 0;

	const auto cores = ctpl::nr_of_cores;
	m_TPool = new ctpl::ThreadPool(cores);
	m_ObjectList = new prims::SceneObjectList();
	m_GpuList = new prims::GpuTriangleList();

	m_Type = type;
	Resize(width, height);
	m_DrawShader = new gl::Shader("shaders/quad.vert", "shaders/quad.frag");
	m_Camera = Camera(m_Width, m_Height, 80.f);

	auto defaultMaterial =
		static_cast<unsigned int>(MaterialManager::GetInstance()->AddMaterial(Material(1.0f, vec3(1.0f), 8.0f)));

	if (m_Type == CPU || m_Type == CPU_RAYTRACER)
	{
		if (scene != nullptr)
			prims::Load(scene, defaultMaterial, glm::vec3(0.0f), 1.0f, m_ObjectList);
		else
			Dragon(m_ObjectList);
	}
	else
	{
		if (scene != nullptr)
			prims::Load(scene, defaultMaterial, glm::vec3(0.0f), 1.0f, m_GpuList);
		else
			Dragon(m_GpuList);

		if (m_GpuList->GetTriangles().empty())
			utils::FatalError(__FILE__, __LINE__, "No triangles for GPU, exiting.", "GPU Init");
	}

	m_Skybox = new core::Surface(skybox != nullptr ? skybox : "models/envmaps/pisa.png");

#if CUBES
	auto *mManager = material::MaterialManager::GetInstance();
	const uint lightMat1 = mManager->AddMaterial(Material(glm::vec3(100.f, 120.f, 80.f), 1.f));
	const uint teapotMaterial = mManager->AddMaterial(Material(.8f, glm::vec3(.3f, .3f, .6f), 2, 1.f, 0.f, vec3(0.f)));
	const uint lightMatDir = mManager->AddMaterial(Material(glm::vec3(1.f), 1.f));
	const uint planeMaterialIdx =
		mManager->AddMaterial(Material(.3f, glm::vec3(0.3f, 0.2f, 0.3f), 2, 1.f, 0.f, vec3(0.f)));

	m_ObjectList->AddLight(new LightDirectional(vec3(0.f, -1.f, -1.f), lightMatDir));
	m_ObjectList->AddLight(new LightPoint(vec3(0.f, 3.5f, 3.f), .3f, lightMat1));

	auto *CubesLarge = new SceneObjectList();
	auto *CubesMedium = new SceneObjectList();
	auto *CubesSmall = new SceneObjectList();

	model::Load("models/cube/cube.obj", teapotMaterial, vec3(0.f, 0.f, 0.f), 1.f, CubesLarge);
	model::Load("models/cube/cube.obj", teapotMaterial, vec3(0.f, 0.f, 0.f), .4f, CubesMedium);
	model::Load("models/cube/cube.obj", teapotMaterial, vec3(0.f, 0.f, 0.f), .1f, CubesSmall);

#if MBVH
	auto *CubesLargeBVH = new MBVHTree(CubesLarge, BVHType::SAH_BINNING, m_TPool);
	auto *CubesMediumBVH = new MBVHTree(CubesMedium, BVHType::SAH_BINNING, m_TPool);
	auto *CubesSmallBVH = new MBVHTree(CubesSmall, BVHType::SAH_BINNING, m_TPool);
#else
	auto *CubesLargeBVH = new StaticBVHTree(CubesLarge, BVHNode::SAH_BINNING, tPool);
	CubesLargeBVH->ConstructBVH();
	auto *CubesMediumBVH = new StaticBVHTree(CubesMedium, BVHNode::SAH_BINNING, tPool);
	CubesMediumBVH->ConstructBVH();
	auto *CubesSmallBVH = new StaticBVHTree(CubesSmall, BVHNode::SAH_BINNING, tPool);
	CubesSmallBVH->ConstructBVH();
#endif

	GameObject *CubeM1 = new GameObject(CubesMediumBVH);
	CubeM1->Move(vec3(0, 1.f, -2.f));
	GameObject *CubeM2 = new GameObject(CubesMediumBVH);
	CubeM2->Move(vec3(2.f, 1.f, 0));
	GameObject *CubeM3 = new GameObject(CubesMediumBVH);
	CubeM3->Move(vec3(0, 1.f, 2.f));
	GameObject *CubeM4 = new GameObject(CubesMediumBVH);
	CubeM4->Move(vec3(-2.f, 1.f, 0));

	std::vector<GameObject *> *childrenM1 = new std::vector<GameObject *>();
	std::vector<GameObject *> *childrenM2 = new std::vector<GameObject *>();
	std::vector<GameObject *> *childrenM3 = new std::vector<GameObject *>();
	std::vector<GameObject *> *childrenM4 = new std::vector<GameObject *>();

	GameObject *CubeM1_1 = new GameObject(CubesSmallBVH);
	CubeM1_1->Move(vec3(0.f, 1.f, -2.5f));
	GameObject *CubeM1_2 = new GameObject(CubesSmallBVH);
	CubeM1_2->Move(vec3(0.5f, 1.f, -2.f));
	GameObject *CubeM1_3 = new GameObject(CubesSmallBVH);
	CubeM1_3->Move(vec3(0.f, 1.f, -1.5f));
	GameObject *CubeM1_4 = new GameObject(CubesSmallBVH);
	CubeM1_4->Move(vec3(-.5f, 1.f, -2.f));
	childrenM1->push_back(CubeM1);
	childrenM1->push_back(CubeM1_1);
	childrenM1->push_back(CubeM1_2);
	childrenM1->push_back(CubeM1_3);
	childrenM1->push_back(CubeM1_4);

	GameObject *CubeM2_1 = new GameObject(CubesSmallBVH);
	CubeM2_1->Move(vec3(2.f, 1.f, -.5f));
	GameObject *CubeM2_2 = new GameObject(CubesSmallBVH);
	CubeM2_2->Move(vec3(2.5f, 1.f, 0));
	GameObject *CubeM2_3 = new GameObject(CubesSmallBVH);
	CubeM2_3->Move(vec3(2.f, 1.f, .5f));
	GameObject *CubeM2_4 = new GameObject(CubesSmallBVH);
	CubeM2_4->Move(vec3(1.5f, 1.f, 0));
	childrenM2->push_back(CubeM2);
	childrenM2->push_back(CubeM2_1);
	childrenM2->push_back(CubeM2_2);
	childrenM2->push_back(CubeM2_3);
	childrenM2->push_back(CubeM2_4);

	GameObject *CubeM3_1 = new GameObject(CubesSmallBVH);
	CubeM3_1->Move(vec3(0, 1.f, 1.5f));
	GameObject *CubeM3_2 = new GameObject(CubesSmallBVH);
	CubeM3_2->Move(vec3(0.5f, 1.f, 2.f));
	GameObject *CubeM3_3 = new GameObject(CubesSmallBVH);
	CubeM3_3->Move(vec3(0, 1.f, 2.5f));
	GameObject *CubeM3_4 = new GameObject(CubesSmallBVH);
	CubeM3_4->Move(vec3(-.5f, 1.f, 2.f));
	childrenM3->push_back(CubeM3);
	childrenM3->push_back(CubeM3_1);
	childrenM3->push_back(CubeM3_2);
	childrenM3->push_back(CubeM3_3);
	childrenM3->push_back(CubeM3_4);

	GameObject *CubeM4_1 = new GameObject(CubesSmallBVH);
	CubeM4_1->Move(vec3(-2.f, 1.f, -0.5f));
	GameObject *CubeM4_2 = new GameObject(CubesSmallBVH);
	CubeM4_2->Move(vec3(-1.5f, 1.f, 0));
	GameObject *CubeM4_3 = new GameObject(CubesSmallBVH);
	CubeM4_3->Move(vec3(-2.f, 1.f, 0.5f));
	GameObject *CubeM4_4 = new GameObject(CubesSmallBVH);
	CubeM4_4->Move(vec3(-2.5f, 1.f, 0));
	childrenM4->push_back(CubeM4);
	childrenM4->push_back(CubeM4_1);
	childrenM4->push_back(CubeM4_2);
	childrenM4->push_back(CubeM4_3);
	childrenM4->push_back(CubeM4_4);

	cubeMother1 = new GameObject(childrenM1);
	cubeMother1->Move(vec3(0, 1.f, -2.f));
	cubeMother2 = new GameObject(childrenM2);
	cubeMother2->Move(vec3(2.f, 1.f, 0));
	cubeMother3 = new GameObject(childrenM3);
	cubeMother3->Move(vec3(0, 1.f, 2.f));
	cubeMother4 = new GameObject(childrenM4);
	cubeMother4->Move(vec3(-2.f, 1.f, 0));

	std::vector<GameObject *> *children = new std::vector<GameObject *>();
	children->push_back(cubeMother1);
	children->push_back(cubeMother2);
	children->push_back(cubeMother3);
	children->push_back(cubeMother4);
	motherGameObject = new GameObject(children);
	motherGameObject->Move(vec3(0.f, 0.f, -5.f));
	gameObjects->push_back(motherGameObject);

#endif

#if TEAPOT
	auto *mManager = material::MaterialManager::GetInstance();

	const unsigned int teapotMaterial =
		static_cast<unsigned int>(mManager->AddMaterial(material::Material(.8f, glm::vec3(.3f, .3f, .6f), 8.0f)));

	auto *teapotList = new SceneObjectList();
	auto *teapotListSmall = new SceneObjectList();

	model::Load("models/teapot.obj", teapotMaterial, vec3(0.f, 0.f, 0.f), 1.f, teapotList);
	model::Load("models/teapot.obj", teapotMaterial, vec3(0.f, 0.f, 0.f), .3f, teapotListSmall);

	auto *teapotBVHStatic = new bvh::StaticBVHTree(teapotList, bvh::BVHType::SAH_BINNING, m_TPool);
	teapotBVHStatic->ConstructBVH();
	auto *teapotBVH = new bvh::MBVHTree(teapotBVHStatic);

	auto *teapotSmallBVHStatic = new StaticBVHTree(teapotListSmall, BVHType::SAH_BINNING, m_TPool);
	teapotSmallBVHStatic->ConstructBVH();
	auto *teapotSmallBVH = new bvh::MBVHTree(teapotSmallBVHStatic);

	auto *teapot0 = new bvh::GameObject(teapotBVH);
	auto *teapot1 = new bvh::GameObject(teapotSmallBVH);
	teapot1->Move(vec3(-2.f, -.5f, -2.f));
	auto *teapot2 = new bvh::GameObject(teapotSmallBVH);
	teapot2->Move(vec3(2.f, -.5f, -2.f));
	auto *teapot3 = new bvh::GameObject(teapotSmallBVH);
	teapot3->Move(vec3(-2.f, -.5f, 2.f));
	auto *teapot4 = new bvh::GameObject(teapotSmallBVH);
	teapot4->Move(vec3(2.f, -.5f, 2.f));
	auto *children = new std::vector<bvh::GameObject *>();
	children->push_back(teapot0);
	children->push_back(teapot1);
	children->push_back(teapot2);
	children->push_back(teapot3);
	children->push_back(teapot4);
	motherGameObject = new bvh::GameObject(children);
	motherGameObject->Rotate(glm::radians(90.f), vec3(0, 1, 0));
	motherGameObject->Move(vec3(0.f, 0.f, -5.f));
	gameObjects->push_back(motherGameObject);
#endif

	switch (m_Type)
	{
	case (GPU):
	{
		std::cout << "Primitive count: " << m_GpuList->GetTriangles().size() << std::endl;
		m_Renderer =
			new core::GpuTracer(m_GpuList, m_OutputTexture[0], m_OutputTexture[1], &m_Camera, m_Skybox, m_TPool);
		m_Renderer->SetMode(core::Mode::ReferenceMicrofacet);
		break;
	}
	case (CPU_RAYTRACER):
	{
		m_Scene = new bvh::TopLevelBVH(m_ObjectList, gameObjects, bvh::BVHType::SAH_BINNING, m_TPool);
		std::cout << "Primitive count: " << m_Scene->GetPrimitiveCount() << std::endl;
		m_Renderer = new core::RayTracer(m_Scene, vec3(0.0f), vec3(0.01f), 16, &m_Camera, m_Width, m_Height);
		m_BVHRenderer = new core::BVHRenderer(m_Scene, &m_Camera, m_Width, m_Height);
		break;
	}
	case (CPU):
	{
	default:
		m_Scene = new bvh::TopLevelBVH(m_ObjectList, gameObjects, bvh::BVHType::SAH_BINNING, m_TPool);
		std::cout << "Primitive count: " << m_Scene->GetPrimitiveCount() << std::endl;
		m_Renderer = new core::PathTracer(m_Scene, m_Width, m_Height, &m_Camera, m_Skybox);
		m_Renderer->SetMode(core::Mode::ReferenceMicrofacet);
		m_BVHRenderer = new core::BVHRenderer(m_Scene, &m_Camera, m_Width, m_Height);
		break;
	}
	}
}

Application::~Application()
{
	if (m_RebuildThread.valid())
		m_RebuildThread.get();

	delete m_OutputTexture[0];
	delete m_OutputTexture[1];
	delete m_DrawShader;
	delete m_BVHRenderer;

	delete m_TPool;
	delete m_Renderer;
	delete m_Skybox;
	delete m_Scene;
}

void Application::Tick(float deltaTime) noexcept
{
	if (!m_DynamicLocked && (m_Type == CPU || m_Type == CPU_RAYTRACER))
	{
		if (m_DBVHBuildTimer.elapsed() > 1000.0f)
		{
			if (m_RebuildingBVH)
			{
				m_RebuildThread.get();
				m_Scene->SwapDynamicTrees();
				m_RebuildingBVH = false;
			}
			else
			{
				m_RebuildingBVH = true;
				m_RebuildThread = m_Scene->ConstructNewDynamicBVHParallel(m_Scene->GetInActiveDynamicTreeIndex());
			}

			m_DBVHBuildTimer.reset();
		}
#if TEAPOT
		motherGameObject->Rotate(glm::radians(deltaTime / 100.f), vec3(0, 1, 0));
		motherGameObject->Move(vec3(0.f, deltaTime / 3000.f, 0.f));
#endif
#if CUBES
		motherGameObject->Rotate(glm::radians(deltaTime / 60.f), vec3(0, 1, 0.3f));
		cubeMother1->Rotate(glm::radians(deltaTime / 10.f), vec3(0.4f, 1, 0));
		cubeMother2->Rotate(glm::radians(deltaTime / 20.f), vec3(0.3f, 1, 0));
		cubeMother3->Rotate(glm::radians(-deltaTime / 30.f), vec3(0.2f, 1, 0));
		cubeMother4->Rotate(glm::radians(-deltaTime / 40.f), vec3(0.1f, 1, 0));
#endif
		m_Scene->UpdateDynamic(*m_Renderer);
	}
}

void Application::HandleKeys(float deltaTime) noexcept
{
	const float movementSpeed = deltaTime * MOVEMENT_SPEED * (m_KeyStatus[GLFW_KEY_LEFT_SHIFT] ? 4.f : 1.f);
	const float rotationSpeed = deltaTime * 0.5f * (m_KeyStatus[GLFW_KEY_LEFT_SHIFT] ? 2.f : 1.f);
	bool resetSamples = false;

	if (m_KeyStatus[GLFW_KEY_LEFT_ALT] && m_KeyStatus[GLFW_KEY_ENTER])
		m_Window->SwitchFullscreen();

	if (!m_MovementLocked)
	{
		if (m_KeyStatus[GLFW_KEY_W])
		{
			m_Camera.MoveForward(movementSpeed);
			resetSamples = true;
		}
		if (m_KeyStatus[GLFW_KEY_S])
		{
			m_Camera.MoveBackward(movementSpeed);
			resetSamples = true;
		}
		if (m_KeyStatus[GLFW_KEY_A])
		{
			m_Camera.MoveLeft(movementSpeed);
			resetSamples = true;
		}
		if (m_KeyStatus[GLFW_KEY_D])
		{
			m_Camera.MoveRight(movementSpeed);
			resetSamples = true;
		}

		if (m_KeyStatus[GLFW_KEY_SPACE])
		{
			m_Camera.MoveUp(movementSpeed);
			resetSamples = true;
		}
		if (m_KeyStatus[GLFW_KEY_LEFT_CONTROL])
		{
			m_Camera.MoveDown(movementSpeed);
			resetSamples = true;
		}

		if (m_KeyStatus[GLFW_KEY_UP])
		{
			m_Camera.RotateDown(rotationSpeed);
			resetSamples = true;
		}
		if (m_KeyStatus[GLFW_KEY_DOWN])
		{
			m_Camera.RotateUp(rotationSpeed);
			resetSamples = true;
		}
		if (m_KeyStatus[GLFW_KEY_RIGHT])
		{
			m_Camera.RotateRight(rotationSpeed);
			resetSamples = true;
		}
		if (m_KeyStatus[GLFW_KEY_LEFT])
		{
			m_Camera.RotateLeft(rotationSpeed);
			resetSamples = true;
		}
		if (m_KeyStatus[GLFW_KEY_R])
		{
			resetSamples = true;
		}

		if (m_KeyStatus[GLFW_KEY_0])
		{
			if (m_MovementTimer.elapsed() > 500.0f)
			{
				m_Renderer->SwitchSkybox();
				resetSamples = true;
				m_MovementTimer.reset();
			}
		}
	}

	if (m_KeyStatus[GLFW_KEY_M])
	{
		SwitchDynamicLocked();
	}

	if (m_KeyStatus[GLFW_KEY_L])
	{
		SwitchMovementLocked();
	}

	if (m_KeyStatus[GLFW_KEY_B])
	{
		SwitchBVHMode();
	}

	if (resetSamples)
	{
		m_Renderer->Reset();
	}
}

void Application::MouseScroll(bool x, bool y)
{
	if (!m_MovementLocked)
	{
		m_Camera.ChangeFOV(m_Camera.GetFOV() + (y ? 1 : -1) * 2);
		m_Renderer->Reset();
	}
}

void Application::MouseScroll(float x, float y)
{
	if (!m_MovementLocked)
	{
		m_Camera.ChangeFOV(m_Camera.GetFOV() + y);
		m_Renderer->Reset();
	}
}

void Application::MouseMove(int x, int y)
{
	if (!m_MovementLocked && m_MouseKeyStatus[GLFW_MOUSE_BUTTON_LEFT])
	{
		m_Camera.ProcessMouse(x, y);
		m_Renderer->Reset();
	}
}

void Application::MouseMove(float x, float y)
{
	if (!m_MovementLocked && m_MouseKeyStatus[GLFW_MOUSE_BUTTON_LEFT])
	{
		m_Camera.ProcessMouse(x, y);
		m_Renderer->Reset();
	}
}

void Application::Draw(float deltaTime)
{
	HandleKeys(deltaTime);

	if (frametimes.size() >= FPS_FRAMES)
		frametimes.pop_front();

	maxFrametime = std::max(maxFrametime, deltaTime);
	frametimes.push_back(deltaTime);
	float avgFrametime = 0.0f;
	std::vector<float> ft(frametimes.size());
	int i = 0;
	for (float fp : frametimes)
	{
		ft[i] = fp;
		avgFrametime += fp;
		i++;
	}

	ImGui::Begin("Information");
	if (ImGui::Button("Reset"))
		m_Renderer->Reset();

	if (m_BVHDebugMode && m_BVHRenderer != nullptr)
	{
		ImGui::Text("Renderer: %s, Mode: BVH", (m_Type == CPU || m_Type == CPU_RAYTRACER) ? "CPU" : "GPU");
		m_BVHRenderer->Render(m_Screen);
	}
	else
	{
		ImGui::Text("Renderer: %s, Mode: %s, Samples: %i", (m_Type == CPU || m_Type == CPU_RAYTRACER) ? "CPU" : "GPU",
					m_Renderer->GetModeString(), m_Renderer->GetSamples());
		m_Renderer->Render(m_Screen);
	}

	if (m_Type == CPU)
		ImGui::Text("BVH Idx: %i", m_Scene->GetActiveDynamicTreeIndex());

	for (float fp : frametimes)
		avgFrametime += fp;
	avgFrametime = (1000.f / (avgFrametime / FPS_FRAMES));

	ImGui::Text("FPS: %f", avgFrametime);
	ImGui::PlotHistogram("", ft.data(), frametimes.size(), 0, "Frametimes", 0.0f, maxFrametime, ImVec2(300, 100));
	//	bool (*items_getter)(void* data, int idx, const char** out_text)
	const auto modes = m_Renderer->GetModes();
	lastMode = renderMode;
	ImGui::ListBox("Mode", &renderMode, modes.data(), modes.size());
	if (lastMode != renderMode)
		m_Renderer->SetMode(modes[renderMode]), m_Renderer->Reset();
	ImGui::End();

	if (m_Type == CPU || m_Type == CPU_RAYTRACER)
		m_OutputTexture[m_tIndex]->flushData();

	m_DrawShader->Bind();
	m_DrawShader->SetInputTexture(0, "glm::vec3", m_OutputTexture[m_tIndex]);
	m_DrawShader->setUniformMatrix4fv("view", glm::mat4(1.f));
	gl::DrawQuad();
	m_DrawShader->Unbind();

	if (m_Type == CPU || m_Type == CPU_RAYTRACER)
	{
		m_tIndex = 1 - m_tIndex;
		m_Screen->SetBuffer(m_OutputTexture[m_tIndex]->mapToPixelBuffer());
	}
}

void Application::Resize(int newWidth, int newHeight)
{
	glFinish();
	m_Camera.SetWidthHeight(newWidth, newHeight);
	m_Width = newWidth;
	m_Height = newHeight;

	delete m_Screen;
	delete m_OutputTexture[0];
	delete m_OutputTexture[1];

	if (m_Type == CPU || m_Type == CPU_RAYTRACER)
	{
		m_Screen = new core::Surface(m_Width, m_Height);
		m_OutputTexture[0] = new gl::Texture(m_Width, m_Height, gl::Texture::SURFACE);
		m_OutputTexture[1] = new gl::Texture(m_Width, m_Height, gl::Texture::SURFACE);
		m_Screen->SetPitch(m_Width);
		m_Screen->SetBuffer(m_OutputTexture[m_tIndex]->mapToPixelBuffer());

		if (m_Renderer)
		{
			delete m_Renderer;
			m_Renderer = new core::PathTracer(m_Scene, m_Width, m_Height, &m_Camera, m_Skybox);
			m_Renderer->SetMode(core::Mode::ReferenceMicrofacet);
			m_Renderer->Resize(m_OutputTexture[m_tIndex]);
		}
	}
	else
	{
		m_OutputTexture[0] = new gl::Texture(m_Width, m_Height, gl::Texture::FLOAT);
		m_OutputTexture[1] = new gl::Texture(m_Width, m_Height, gl::Texture::FLOAT);
		if (!m_Screen)
			m_Screen = new core::Surface(m_Width, m_Height);

		if (m_Renderer)
		{
			((core::GpuTracer *)m_Renderer)->Resize(m_OutputTexture[0]);
			((core::GpuTracer *)m_Renderer)->SetOutput(m_OutputTexture[0], m_OutputTexture[1]);
		}
	}
}