#include <SDL2/SDL.h>

#include "Utils/Messages.h"
#include "Utils/Template.h"

namespace utils
{
void FatalError(const char *file, int line, const char *message)
{
    std::string msg = file;
    msg += ", line ";
    msg += line;
    msg += ":\n";
    msg += message;
    const auto *output = msg.c_str();

    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", output,
                             gSDLContext);
    exit(0);
}

void FatalError(const char *file, int line, const char *message,
                const char *context)
{
    std::string msg = "Context: ";
    msg += context;
    msg += ", ";
    msg += file;
    msg += ", line ";
    msg += line;
    msg += ":\n";
    msg += message;
    const auto *output = msg.c_str();

    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", output,
                             gSDLContext);
    exit(0);
}

void WarningMessage(const char *file, int line, const char *message)
{
    std::string msg = file;
    msg += ", line ";
    msg += line;
    msg += ":\n";
    msg += message;
    const auto *output = msg.c_str();

    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Warning", output,
                             gSDLContext);
}

void WarningMessage(const char *file, int line, const char *message,
                    const char *context)
{
    std::string msg = "Context: ";
    msg += context;
    msg += ", ";
    msg += file;
    msg += ", line ";
    msg += line;
    msg += ":\n";
    msg += message;
    const auto *output = msg.c_str();

    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Warning", output,
                             gSDLContext);
}
}; // namespace utils