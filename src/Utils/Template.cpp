#include "Utils/Template.h"

#include "Application.h"
#include "Shared.h"
#include "Utils/Timer.h"

#define SCRWIDTH 1024
#define SCRHEIGHT 768

int main(int argc, char *argv[])
{
    printf("Application started.\n");
    SDL_Init(SDL_INIT_VIDEO);

    bool oFullScreen = false;

    for (int i = 2; i < argc; i++)
    {
        const std::string str = argv[i];
        if (str == "--fullscreen" || str == "-f")
            oFullScreen = true;
    }

    unsigned int flags = oFullScreen ? SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN : SDL_WINDOW_OPENGL;

    gSDLContext =
        SDL_CreateWindow("Path tracer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCRWIDTH, SCRHEIGHT, flags);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GLContext glContext = SDL_GL_CreateContext(gSDLContext);
    SDL_GL_MakeCurrent(gSDLContext, glContext);

    glewExperimental = (GLboolean) true;
    GLenum error = glewInit();
    if (error != GLEW_OK)
    {
        std::cout << "Could not init GLEW: " << glewGetErrorString(error) << std::endl;
        exit(EXIT_FAILURE);
    }

    glViewport(0, 0, SCRWIDTH, SCRHEIGHT);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    SDL_GL_SetSwapInterval(0);
    SDL_SetRelativeMouseMode(SDL_FALSE);
    SDL_CaptureMouse(SDL_FALSE);

    SDL_PumpEvents();
    SDL_SetWindowSize(gSDLContext, SCRWIDTH, SCRHEIGHT);

    int exitApp = 0;
    auto app = new Application(gSDLContext, RendererType::CPU, SCRWIDTH, SCRHEIGHT, argc >= 2 ? argv[1] : nullptr);

    Timer t, drawTimer;

    while (!exitApp)
    {
        const float elapsed = t.elapsed();
        t.reset();
        app->Tick(elapsed);
        app->Draw(elapsed);
        SDL_GL_SwapWindow(gSDLContext);

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                exitApp = 1;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    exitApp = 1;
                app->KeyDown(event.key.keysym.scancode);
                break;
            case SDL_KEYUP:
                app->KeyUp(event.key.keysym.scancode);
                break;
            case SDL_MOUSEMOTION:
                app->MouseMove(event.motion.xrel, event.motion.yrel);
                break;
            case SDL_MOUSEBUTTONUP:
                app->MouseUp(event.button.button);
                break;
            case SDL_MOUSEBUTTONDOWN:
                app->MouseDown(event.button.button);
                break;
            case SDL_MOUSEWHEEL:
                app->MouseScroll(event.wheel.x > 0, event.wheel.y > 0);
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                {
                    const int width = event.window.data1;
                    const int height = event.window.data2;
                    if (width != app->GetWidth() || height != app->GetHeight())
                    {
                        glViewport(0, 0, event.window.data1, event.window.data2);
                        app->Resize(event.window.data1, event.window.data2);
                    }
                }
                break;
            default:
                break;
            }
        }
    }

    delete app;
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(gSDLContext);
    SDL_Quit();
    return EXIT_SUCCESS;
}

uint Vec3ToInt(const glm::vec3 &vec)
{
    const float fr = sqrtf(glm::min(glm::max(vec.r, 0.0f), 1.0f)) * 255.99f;
    const float fg = sqrtf(glm::min(glm::max(vec.g, 0.0f), 1.0f)) * 255.99f;
    const float fb = sqrtf(glm::min(glm::max(vec.b, 0.0f), 1.0f)) * 255.99f;

    const auto r = uint(fr) << 0;
    const auto g = uint(fg) << 8;
    const auto b = uint(fb) << 16;

    return r + g + b;
}

glm::vec3 IntToVec3(const uint &u)
{
    const unsigned int red = u & 0xFF;
    const unsigned int green = (u >> 8) & 0xFF;
    const unsigned int blue = (u >> 16) & 0xFF;
    return {float(red) / 255.0f, float(green) / 255.0f, float(blue) / 255.0f};
}