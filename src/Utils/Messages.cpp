#include "Utils/Messages.h"
#include "boxer/boxer.h"

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
	boxer::show(output, "Error");
	exit(0);
}

void FatalError(const char *file, int line, const char *message, const char *context)
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
	boxer::show(output, "Error");
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
	boxer::show(output, "Warning");
}

void WarningMessage(const char *file, int line, const char *message, const char *context)
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
	boxer::show(output, "Warning");
}
}; // namespace utils