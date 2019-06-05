#if defined(WIN32) || defined(MSVC)
#include <Windows.h>
#endif

#include <map>

#include "CL/Buffer.h"
#include "CL/OpenCL.h"
#include "Device.h"
#include "GL/Texture.h"
#include "Shared.h"

namespace cl
{
// static members of Kernel class
bool Kernel::m_Initialized = false;
bool Kernel::canDoInterop = true;
char *Kernel::m_Log = nullptr;
cl_context Kernel::m_Context;
cl_command_queue Kernel::m_Queue;
cl_device_id Kernel::m_Device;

// source file information
static int sourceFiles = 0;
static char *sourceFile[64]; // yup, ugly constant

using namespace std;
using namespace glm;
using namespace utils;

static cl_device_id getFirstDevice(cl_context context)
{
	size_t dataSize;
	clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, nullptr, &dataSize);
	std::vector<cl_device_id> devices = std::vector<cl_device_id>(dataSize / sizeof(cl_device_id));
	clGetContextInfo(context, CL_CONTEXT_DEVICES, dataSize, devices.data(), nullptr);
	cl_device_id first = devices[0];
	return first;
}

static cl_int getPlatformID(cl_platform_id *platform)
{
	char chBuffer[1024];
	cl_uint numPlatforms, devCount;
	cl_platform_id *clPlatformIDs;
	*platform = nullptr;

	CHECKCL(clGetPlatformIDs(0, nullptr, &numPlatforms));
	if (numPlatforms == 0)
		FatalError(__FILE__, __LINE__, "No valid OpenCL platforms found.", "OpenCL Init");

	clPlatformIDs = (cl_platform_id *)malloc(numPlatforms * sizeof(cl_platform_id));
	CHECKCL(clGetPlatformIDs(numPlatforms, clPlatformIDs, nullptr));

	cl_uint deviceType[2] = {CL_DEVICE_TYPE_GPU, CL_DEVICE_TYPE_CPU};
	const char *deviceOrder[1][3] = {{"NVIDIA", "AMD", ""}};
	printf("available OpenCL platforms:\n");
	for (cl_uint i = 0; i < numPlatforms; ++i)
	{
		CHECKCL(clGetPlatformInfo(clPlatformIDs[i], CL_PLATFORM_NAME, 1024, &chBuffer, nullptr));
		printf("#%i: %s\n", i, chBuffer);
	}

	for (cl_uint j = 0; j < 2; j++)
	{
		for (int k = 0; k < 3; k++)
		{
			for (cl_uint i = 0; i < numPlatforms; ++i)
			{
				cl_int error = clGetDeviceIDs(clPlatformIDs[i], deviceType[j], 0, nullptr, &devCount);
				if ((error != CL_SUCCESS) || (devCount == 0))
					continue;

				CHECKCL(clGetPlatformInfo(clPlatformIDs[i], CL_PLATFORM_NAME, 1024, &chBuffer, nullptr));
				if (deviceOrder[j][k][0])
					if (!strstr(chBuffer, deviceOrder[j][k]))
						continue;

				printf("OpenCL device: %s\n", chBuffer);
				*platform = clPlatformIDs[i];
				j = 2, k = 3;
				break;
			}
		}
	}

	free(clPlatformIDs);
	return CL_SUCCESS;
}

static char *loadSource(const char *file, size_t *size)
{
	string source;
	// extract path from source file name
	char path[2048];
	strcpy(path, file);
	char *marker = path;
	char *fileName = (char *)file;

	while (strstr(marker + 1, "\\"))
		marker = strstr(marker + 1, "\\");
	while (strstr(marker + 1, "/"))
		marker = strstr(marker + 1, "/");
	while (strstr(fileName + 1, "\\"))
		fileName = strstr(fileName + 1, "\\");
	while (strstr(fileName + 1, "/"))
		fileName = strstr(fileName + 1, "/");
	if (fileName != file)
		fileName++;
	sourceFile[sourceFiles] = new char[strlen(fileName) + 1];
	strcpy(sourceFile[sourceFiles], fileName);
	*marker = 0;

	// load source file
	FILE *f = fopen(file, "r");
	if (!f)
		FatalError(__FILE__, __LINE__, file, "loadSource");
	char line[8192];
	int lineNr = 0, currentFile = ((sourceFiles + 1) % 64);
	while (!feof(f))
	{
		line[0] = 0;
		fgets(line, 8190, f);
		lineNr++;
		// clear source file line
		while (line[0])
		{
			if (line[strlen(line) - 1] > 32)
				break;
			line[strlen(line) - 1] = 0;
		}

		// expand error commands
		char *err = strstr(line, "Error(");
		if (err)
		{
			char rem[8192], cmd[128];
			strcpy(rem, err + 6);
			*err = 0;
			sprintf(cmd, "Error_( %i, %i,", currentFile, lineNr);
			strcat(line, cmd);
			strcat(line, rem);
		}

		// expand assets
		char *as = strstr(line, "Assert(");
		if (as)
		{
			char rem[8192], cmd[128];
			strcpy(rem, as + 7);
			*as = 0;
			sprintf(cmd, "Assert_( %i, %i,", currentFile, lineNr);
			strcat(line, cmd);
			strcat(line, rem);
		}

		// handle include files
		char *inc = strstr(line, "#include");
		if (inc)
		{
			char *start = strstr(inc, "\"");
			if (!start)
				FatalError(__FILE__, __LINE__, "Preprocessor error in #include statement line", "loadSource");
			char *end = strstr(start + 1, "\"");
			if (!end)
				FatalError(__FILE__, __LINE__, "Preprocessor error in #include statement line", "loadSource");
			char file[2048];
			*end = 0;
			strcpy(file, path);
			strcat(file, "/");
			strcat(file, start + 1);
			char *incText = loadSource(file, size);
			source.append(incText);
		}
		else
		{
			source.append(line);
			source.append("\n");
		}
	}

	*size = strlen(source.c_str());
	char *t = (char *)malloc(*size + 1);
	strcpy(t, source.c_str());
	return t;
}

Kernel::Kernel(const char *file, const char *entryPoint, std::tuple<size_t, size_t, size_t> workSize)
{
	if (!m_Initialized)
	{
		if (!initCL())
			FatalError(__FILE__, __LINE__, "Failed to initialize OpenCL");
		m_Initialized = true;
	}
	size_t size;
	cl_int error;
	char *source = loadSource(file, &size);
	m_Program = clCreateProgramWithSource(m_Context, 1, (const char **)&source, &size, &error);
	CHECKCL(error);
	error = clBuildProgram(m_Program, 0, nullptr,
				   "-cl-fast-relaxed-math -cl-mad-enable "
				   "-cl-denorms-are-zero -cl-no-signed-zeros "
				   "-cl-unsafe-math-optimizations -cl-finite-math-only "
				   "-cl-strict-aliasing -cl-kernel-arg-info",
				   nullptr, nullptr);

	if (error != CL_SUCCESS)
	{
		auto log = std::vector<char>(100 * 1024);
		clGetProgramBuildInfo(m_Program, getFirstDevice(m_Context), CL_PROGRAM_BUILD_LOG, 100 * 1024, log.data(),
							  nullptr);

		char buffer[256];
		sprintf(buffer, "Build error, file: %s", file);
		FatalError(__FILE__, __LINE__, log.data(), buffer);
	}

	m_Kernel = clCreateKernel(m_Program, entryPoint, &error);
	CheckCL(error, __FILE__, __LINE__);

	m_WorkSize[0] = std::get<0>(workSize);
	m_WorkSize[1] = std::get<1>(workSize);
	m_WorkSize[2] = std::get<2>(workSize);

	m_LocalSize[0] = 16;
	m_LocalSize[1] = 16;
	m_LocalSize[2] = 1;
}

Kernel::~Kernel()
{
	if (m_Kernel)
		clReleaseKernel(m_Kernel);
	if (m_Program)
		clReleaseProgram(m_Program);
	if (m_Initialized)
	{
		if (m_Queue)
			clReleaseCommandQueue(m_Queue);
		if (m_Context)
			clReleaseContext(m_Context);
		m_Initialized = false;
	}
}

bool Kernel::initCL()
{
	cl_platform_id platform;
	std::vector<cl_device_id> deviceIDs;
	cl_uint devCount;
	cl_int error;

	if (!CHECKCL(error = getPlatformID(&platform)))
		return false;

#ifdef __APPLE__
	if (!CHECKCL(error = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, nullptr, &devCount)))
		return false;

	deviceIDs.resize(devCount);
	if (!CHECKCL(error = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, devCount, deviceIDs.data(), nullptr)))
		return false;
#else
	if (!CheckCL(error = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, nullptr, &devCount), __FILE__, __LINE__))
		return false;
	deviceIDs.resize(devCount);
	if (!CheckCL(error = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, devCount, deviceIDs.data(), nullptr), __FILE__,
				 __LINE__))
		return false;
#endif
	std::vector<cl_device_id> compatibleDevices;

	for (unsigned int i = 0; i < devCount; ++i)
	{
		size_t extensionSize;
		CHECKCL(error = clGetDeviceInfo(deviceIDs[i], CL_DEVICE_EXTENSIONS, 0, nullptr, &extensionSize));
		if (extensionSize > 0)
		{
			std::vector<char> extensions(extensionSize / sizeof(char));
			CHECKCL(error = clGetDeviceInfo(deviceIDs[i], CL_DEVICE_EXTENSIONS, extensionSize, extensions.data(),
											&extensionSize));
			std::string deviceExtensions(extensions.data());
			std::transform(deviceExtensions.begin(), deviceExtensions.end(), deviceExtensions.begin(), ::tolower);

			size_t oldPos = 0;
			size_t spacePos = deviceExtensions.find(' ', oldPos); // extensions string is space delimited

#ifdef __APPLE__
			const char *neededProp = "cl_apple_gl_sharing";
#else
			const char *neededProp = "cl_khr_gl_sharing";
#endif

			while (spacePos != std::string::npos)
			{
				if (strcmp(neededProp, deviceExtensions.substr(oldPos, spacePos - oldPos).c_str()) == 0)
				{
					compatibleDevices.push_back(deviceIDs[i]);
					break;
				}
				do
				{
					oldPos = spacePos + 1;
					spacePos = deviceExtensions.find(' ', oldPos);
				} while (spacePos == oldPos);
			}
		}
	}

	if (compatibleDevices.empty())
	{
		std::cout << "No device found that supports CL/GL context sharing." << std::endl;
		return false;
	}

	std::cout << "Using CL-GL Interop." << std::endl;

#ifdef WIN32
	cl_context_properties props[] = {CL_GL_CONTEXT_KHR,
									 (cl_context_properties)wglGetCurrentContext(),
									 CL_WGL_HDC_KHR,
									 (cl_context_properties)wglGetCurrentDC(),
									 CL_CONTEXT_PLATFORM,
									 (cl_context_properties)platform,
									 0};
#elif defined(__APPLE__)
	CGLContextObj kCGLContext = CGLGetCurrentContext();
	CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext);
	cl_context_properties props[] = {CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE,
									 (cl_context_properties)kCGLShareGroup, CL_CONTEXT_PLATFORM,
									 (cl_context_properties)platform, 0};
#else // Linux
	cl_context_properties props[] = {CL_GL_CONTEXT_KHR,
									 (cl_context_properties)glXGetCurrentContext(),
									 CL_GLX_DISPLAY_KHR,
									 (cl_context_properties)glXGetCurrentDisplay(),
									 CL_CONTEXT_PLATFORM,
									 (cl_context_properties)platform,
									 0};
#endif

	std::vector<Device> devices(compatibleDevices.size());
	for (unsigned int i = 0; i < compatibleDevices.size(); i++)
		devices[i] = Device(compatibleDevices[i]);

	auto calculateScore = [](const Device &dev) -> float {
		float score = 0.0f;

		if (!dev.isAvailable())
			return score;

		score += static_cast<float>(dev.getGlobalMemorySize()) / powf(1024.0f, 3.0f); // convert to GB
		score += static_cast<float>(dev.getComputeUnits()) / 20.0f;
		score *= (dev.getType() == Device::Type::GPU ? 10.0f : 1.0f);

		return score;
	};

	std::sort(devices.begin(), devices.end(), [&calculateScore](const Device &dev1, const Device &dev2) {
		return calculateScore(dev1) > calculateScore(dev2);
	});

	bool validDeviceFound = false;
	for (const auto &dev : devices)
	{
		canDoInterop = true;
		m_Device = dev.getID();
		m_Context = clCreateContext(props, 1, &m_Device, nullptr, nullptr, &error);
		if (!CHECKCL(error)) // try next device
			continue;
		m_Queue = clCreateCommandQueue(m_Context, m_Device, 0, &error);
		if (error == CL_INVALID_DEVICE)
		{
			std::cout << "Could not initialize command queue with " << dev.getName() << std::endl;
			clReleaseContext(m_Context);
			continue;
		}

		validDeviceFound = true;
		std::cout << "OpenCL context created." << std::endl;
		std::cout << "Using device:" << std::endl;
		std::cout << "\tName: " << dev.getName() << std::endl;
		std::cout << "\tGlobal memory: " << dev.getGlobalMemorySize() / (powf(1024.0f, 3.0f)) << " GB" << std::endl;
		std::cout << "\tCompute units: " << dev.getComputeUnits() << std::endl;
		break;
	}

	Kernel::m_Initialized = validDeviceFound;
	return validDeviceFound;
}

cl_int Kernel::run()
{
	glFinish();
	return clEnqueueNDRangeKernel(m_Queue, m_Kernel, 2, 0, m_WorkSize, m_LocalSize, 0, 0, 0);
}

cl_int Kernel::run(cl_mem *buffers, int count)
{
	cl_int err;

	glFinish();
	if (Kernel::canDoInterop)
	{
		err = clEnqueueAcquireGLObjects(m_Queue, count, buffers, 0, 0, 0);
		err = clEnqueueNDRangeKernel(m_Queue, m_Kernel, 2, nullptr, m_WorkSize, m_LocalSize, 0, 0, 0);
		err = clEnqueueReleaseGLObjects(m_Queue, count, buffers, 0, 0, 0);
	}
	else
		err = clEnqueueNDRangeKernel(m_Queue, m_Kernel, 2, nullptr, m_WorkSize, m_LocalSize, 0, 0, 0);

	return err;
}

cl_int Kernel::run(cl::TextureBuffer *buffer)
{
	cl_int err = CL_SUCCESS;
	glFinish();
	if (Kernel::canDoInterop)
	{
		err = clEnqueueAcquireGLObjects(m_Queue, 1, buffer->getDevicePtr(), 0, 0, 0);
		err = clEnqueueNDRangeKernel(m_Queue, m_Kernel, 2, nullptr, m_WorkSize, m_LocalSize, 0, 0, 0);
		err = clEnqueueReleaseGLObjects(m_Queue, 1, buffer->getDevicePtr(), 0, 0, 0);
	}
	else
		err = clEnqueueNDRangeKernel(m_Queue, m_Kernel, 2, nullptr, m_WorkSize, m_LocalSize, 0, 0, 0);

	return err;
}

cl_int Kernel::run(size_t count) { return clEnqueueNDRangeKernel(m_Queue, m_Kernel, 1, 0, &count, 0, 0, 0, 0); }

cl_int Kernel::syncQueue() { return clFinish(m_Queue); }

cl_int Kernel::setBuffer(int idx, cl::TextureBuffer *buffer)
{
	return clSetKernelArg(m_Kernel, idx, sizeof(cl_mem), buffer->getDevicePtr());
}
} // namespace cl