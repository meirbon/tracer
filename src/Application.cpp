#include "Application.h"
#include "Materials/MaterialManager.h"

#define FPS_FRAMES 100
#define TEAPOT 0
#define CUBES 0

static float frametimes[FPS_FRAMES]{};
static int frametimeIndex = 0;
static char msText[32], fpsText[64], infoText[32], sampleText[128];
auto *gameObjects = new std::vector<bvh::GameObject *>();

bvh::GameObject *motherGameObject;
#if CUBES
bvh::GameObject *cubeMother1, *cubeMother2, *cubeMother3, *cubeMother4;
#endif

Application::Application(SDL_Window *window, RendererType type, int width, int height, const char *scene,
                         const char *skybox)
    : m_Type(type), m_tIndex(0), m_Width(width), m_Height(height), m_Window(window)
{
    using namespace material;
    using namespace bvh;

    m_TPool = new ctpl::ThreadPool(ctpl::nr_of_cores);
    m_ObjectList = new SceneObjectList();
    m_GpuList = new GpuTriangleList();

    m_Type = type;
    Resize(width, height);
    m_DrawShader = new Shader("shaders/quad.vert", "shaders/quad.frag");
    m_Camera = Camera(m_Width, m_Height, 80.f);

    unsigned int defaultMaterial = static_cast<unsigned int>(
        material::MaterialManager::GetInstance()->AddMaterial(material::Material(1.0f, vec3(1.0f), 8.0f)));

    if (m_Type == CPU || m_Type == CPU_RAYTRACER)
    {
        if (scene != nullptr)
            model::Load(scene, defaultMaterial, glm::vec3(0.0f), 1.0f, m_ObjectList);
        else
            Micromaterials(m_ObjectList);
    }
    else
    {
        if (scene != nullptr)
            model::Load(scene, defaultMaterial, glm::vec3(0.0f), 1.0f, m_GpuList);
        else
            Dragon(m_GpuList);
            //Micromaterials(m_GpuList);

        if (m_GpuList->GetTriangles().empty())
            utils::FatalError(__FILE__, __LINE__, "No triangles for GPU, exiting.", "GPU Init");
    }

    m_Skybox = new Surface(skybox != nullptr ? skybox : "models/envmaps/pisa.png");

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
        m_Renderer = new GpuTracer(m_GpuList, m_OutputTexture[0], m_OutputTexture[1], &m_Camera, m_Skybox, m_TPool);
        m_Renderer->SetMode(Mode::ReferenceMicrofacet);
        break;
    }
    case (CPU_RAYTRACER):
    {
        m_Scene = new bvh::TopLevelBVH(m_ObjectList, gameObjects, bvh::BVHType::SAH_BINNING, m_TPool);
        std::cout << "Primitive count: " << m_Scene->GetPrimitiveCount() << std::endl;
        m_Renderer = new RayTracer(m_Scene, vec3(0.0f), vec3(0.01f), 16, &m_Camera, m_Width, m_Height);
        m_BVHRenderer = new BVHRenderer(m_Scene, &m_Camera, m_Width, m_Height);
        break;
    }
    case (CPU):
    {
    default:
        m_Scene = new bvh::TopLevelBVH(m_ObjectList, gameObjects, bvh::BVHType::SAH_BINNING, m_TPool);
        std::cout << "Primitive count: " << m_Scene->GetPrimitiveCount() << std::endl;
        m_Renderer = new PathTracer(m_Scene, m_Width, m_Height, &m_Camera, m_Skybox);
        m_Renderer->SetMode(Mode::ReferenceMicrofacet);
        m_BVHRenderer = new BVHRenderer(m_Scene, &m_Camera, m_Width, m_Height);
        break;
    }
    }

    for (auto &fp : frametimes)
    {
        fp = 1.0f;
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
    const float movementSpeed = deltaTime * MOVEMENT_SPEED * (m_KeyStatus[SDL_SCANCODE_LSHIFT] ? 4.f : 1.f);
    const float rotationSpeed = deltaTime * 0.5f * (m_KeyStatus[SDL_SCANCODE_LSHIFT] ? 2.f : 1.f);
    bool resetSamples = false;

    if (!m_MovementLocked)
    {
        if (m_KeyStatus[SDL_SCANCODE_W])
        {
            m_Camera.MoveForward(movementSpeed);
            resetSamples = true;
        }
        if (m_KeyStatus[SDL_SCANCODE_S])
        {
            m_Camera.MoveBackward(movementSpeed);
            resetSamples = true;
        }
        if (m_KeyStatus[SDL_SCANCODE_A])
        {
            m_Camera.MoveLeft(movementSpeed);
            resetSamples = true;
        }
        if (m_KeyStatus[SDL_SCANCODE_D])
        {
            m_Camera.MoveRight(movementSpeed);
            resetSamples = true;
        }

        if (m_KeyStatus[SDL_SCANCODE_SPACE])
        {
            m_Camera.MoveUp(movementSpeed);
            resetSamples = true;
        }
        if (m_KeyStatus[SDL_SCANCODE_LCTRL])
        {
            m_Camera.MoveDown(movementSpeed);
            resetSamples = true;
        }

        if (m_KeyStatus[SDL_SCANCODE_UP])
        {
            m_Camera.RotateDown(rotationSpeed);
            resetSamples = true;
        }
        if (m_KeyStatus[SDL_SCANCODE_DOWN])
        {
            m_Camera.RotateUp(rotationSpeed);
            resetSamples = true;
        }
        if (m_KeyStatus[SDL_SCANCODE_RIGHT])
        {
            m_Camera.RotateRight(rotationSpeed);
            resetSamples = true;
        }
        if (m_KeyStatus[SDL_SCANCODE_LEFT])
        {
            m_Camera.RotateLeft(rotationSpeed);
            resetSamples = true;
        }
        if (m_KeyStatus[SDL_SCANCODE_R])
        {
            resetSamples = true;
        }

        if (m_KeyStatus[SDL_SCANCODE_0])
        {
            if (m_MovementTimer.elapsed() > 500.0f)
            {
                m_Renderer->SwitchSkybox();
                resetSamples = true;
                m_MovementTimer.reset();
            }
        }
        else if (m_KeyStatus[SDL_SCANCODE_1])
        {
            m_Renderer->SetMode(Mode::Reference);
            resetSamples = true;
        }
        else if (m_KeyStatus[SDL_SCANCODE_2])
        {
            m_Renderer->SetMode(Mode::NEE);
            resetSamples = true;
        }
        else if (m_KeyStatus[SDL_SCANCODE_3])
        {
            m_Renderer->SetMode(Mode::IS);
            resetSamples = true;
        }
        else if (m_KeyStatus[SDL_SCANCODE_4])
        {
            m_Renderer->SetMode(Mode::NEE_IS);
            resetSamples = true;
        }
        else if (m_KeyStatus[SDL_SCANCODE_5])
        {
            m_Renderer->SetMode(Mode::NEE_MIS);
            resetSamples = true;
        }
        else if (m_KeyStatus[SDL_SCANCODE_6])
        {
            m_Renderer->SetMode(Mode::ReferenceMicrofacet);
            resetSamples = true;
        }
        else if (m_KeyStatus[SDL_SCANCODE_7])
        {
            m_Renderer->SetMode(Mode::NEEMicrofacet);
            resetSamples = true;
        }
    }

    if (m_KeyStatus[SDL_SCANCODE_M])
    {
        SwitchDynamicLocked();
    }

    if (m_KeyStatus[SDL_SCANCODE_L])
    {
        SwitchMovementLocked();
    }

    if (m_KeyStatus[SDL_SCANCODE_B])
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

void Application::MouseMove(int x, int y)
{
    if (!m_MovementLocked && m_MouseKeyStatus[SDL_BUTTON_LEFT])
    {
        m_Camera.ProcessMouse(x, y);
        m_Renderer->Reset();
    }
}

void Application::Draw(float deltaTime)
{
    HandleKeys(deltaTime);

    if (m_BVHDebugMode && m_BVHRenderer != nullptr)
    {
        m_BVHRenderer->Render(m_Screen);
    }
    else
    {
        m_Renderer->Render(m_Screen);

        if (m_Type == CPU)
        {
            std::sprintf(msText, "Frametime: %.2f", deltaTime);
            std::sprintf(infoText, "FOV: %.0f", m_Camera.GetFOV());
            sprintf(sampleText, "Samples: %i, Mode: %s, BVH: %i", ((PathTracer *)m_Renderer)->GetSamples(),
                    ((PathTracer *)m_Renderer)->GetModeString(), m_Scene->GetActiveDynamicTreeIndex());

            frametimes[frametimeIndex] = deltaTime;
            frametimeIndex = (frametimeIndex + 1) % FPS_FRAMES;
            float avgFrametime = 0;

            for (float fp : frametimes)
            {
                avgFrametime += fp;
            }

            avgFrametime = 1000.f / (avgFrametime / FPS_FRAMES);
            std::sprintf(fpsText, "FPS: %.2f", avgFrametime);

            m_Screen->Print(msText, 0, 2, 0xFFFFFF);
            m_Screen->Print(fpsText, 0, 10, 0xFFFFFF);
            m_Screen->Print(infoText, 0, 18, 0xFFFFFF);
            m_Screen->Print(sampleText, 0, 26, 0xFFFFFF);
        }
        else
        {
            frametimes[frametimeIndex] = deltaTime;
            frametimeIndex = (frametimeIndex + 1) % FPS_FRAMES;
            float avgFrametime = 0;
            for (float fp : frametimes)
            {
                avgFrametime += fp;
            }

            avgFrametime = (1000.f / (avgFrametime / FPS_FRAMES));
            std::sprintf(fpsText, "FPS: %.2f, Mode: %s", avgFrametime, m_Renderer->GetModeString());

            SDL_SetWindowTitle(m_Window, fpsText);
        }
    }

    if (m_Type == CPU || m_Type == CPU_RAYTRACER)
        m_OutputTexture[m_tIndex]->flushData();

    m_DrawShader->Bind();
    m_DrawShader->SetInputTexture(0, "glm::vec3", m_OutputTexture[m_tIndex]);
    m_DrawShader->setUniformMatrix4fv("view", glm::mat4(1.f));
    DrawQuad();
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
        m_Screen = new Surface(m_Width, m_Height);
        m_OutputTexture[0] = new Texture(m_Width, m_Height, Texture::SURFACE);
        m_OutputTexture[1] = new Texture(m_Width, m_Height, Texture::SURFACE);
        m_Screen->SetPitch(m_Width);
        m_Screen->SetBuffer(m_OutputTexture[m_tIndex]->mapToPixelBuffer());

        if (m_Renderer)
        {
            delete m_Renderer;
            m_Renderer = new PathTracer(m_Scene, m_Width, m_Height, &m_Camera, m_Skybox);
            m_Renderer->SetMode(Mode::ReferenceMicrofacet);
            m_Renderer->Resize(m_OutputTexture[m_tIndex]);
        }
    }
    else
    {
        m_OutputTexture[0] = new Texture(m_Width, m_Height, Texture::FLOAT);
        m_OutputTexture[1] = new Texture(m_Width, m_Height, Texture::FLOAT);
        if (!m_Screen)
            m_Screen = new Surface(m_Width, m_Height);

        if (m_Renderer)
        {
            ((GpuTracer *)m_Renderer)->Resize(m_OutputTexture[0]);
            ((GpuTracer *)m_Renderer)->outputTexture[0] = m_OutputTexture[0];
            ((GpuTracer *)m_Renderer)->outputTexture[1] = m_OutputTexture[1];
        }
    }
}