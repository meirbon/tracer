#pragma once

#include <string>

namespace utils
{
void FatalError(const char *file, int line, const char *message);
void FatalError(const char *file, int line, const char *message,
                const char *context);
void WarningMessage(const char *file, int line, const char *message);
void WarningMessage(const char *file, int line, const char *message,
                    const char *context);
}; // namespace utils