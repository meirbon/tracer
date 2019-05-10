#if defined(WIN32) || defined(MSVC)
#include <Windows.h>
#endif

#include "CL/Buffer.h"
#include "CL/OpenCL.h"
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
	cl_device_id *devices;
	clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, nullptr, &dataSize);
	devices = (cl_device_id *)malloc(dataSize);
	clGetContextInfo(context, CL_CONTEXT_DEVICES, dataSize, devices, nullptr);
	cl_device_id first = devices[0];
	free(devices);
	return first;
}

static cl_int getPlatformID(cl_platform_id *platform)
{
	char chBuffer[1024];
	cl_uint num_platforms, devCount;
	cl_platform_id *clPlatformIDs;
	*platform = nullptr;

	CheckCL(clGetPlatformIDs(0, nullptr, &num_platforms), __FILE__, __LINE__);
	if (num_platforms == 0)
	{
		FatalError(__FILE__, __LINE__, "No valid OpenCL platforms found.", "OpenCL Init");
	}

	clPlatformIDs = (cl_platform_id *)malloc(num_platforms * sizeof(cl_platform_id));
	CHECKCL(clGetPlatformIDs(num_platforms, clPlatformIDs, nullptr));
#ifdef USE_CPU_DEVICE
	cl_uint deviceType[2] = {CL_DEVICE_TYPE_CPU, CL_DEVICE_TYPE_CPU};
	char *deviceOrder[2][3] = {{"", "", ""}, {"", "", ""}};
#else
	//	cl_uint deviceType[2] = { CL_DEVICE_TYPE_GPU, CL_DEVICE_TYPE_CPU };
	cl_uint deviceType[1] = {CL_DEVICE_TYPE_GPU};
	//	const char* deviceOrder[2][3] = {{"NVIDIA", "AMD", "" }, {"", "", "" }
	//};
	const char *deviceOrder[1][3] = {{"NVIDIA", "AMD", ""}};
#endif
	printf("available OpenCL platforms:\n");
	for (cl_uint i = 0; i < num_platforms; ++i)
	{
		CHECKCL(clGetPlatformInfo(clPlatformIDs[i], CL_PLATFORM_NAME, 1024, &chBuffer, nullptr));
		printf("#%i: %s\n", i, chBuffer);
	}
	for (cl_uint j = 0; j < 2; j++)
	{
		for (int k = 0; k < 3; k++)
		{
			for (cl_uint i = 0; i < num_platforms; ++i)
			{
				cl_int error = clGetDeviceIDs(clPlatformIDs[i], deviceType[j], 0, nullptr, &devCount);
				if ((error != CL_SUCCESS) || (devCount == 0))
				{
					continue;
				}

				CHECKCL(clGetPlatformInfo(clPlatformIDs[i], CL_PLATFORM_NAME, 1024, &chBuffer, nullptr));
				if (deviceOrder[j][k][0])
				{
					if (!strstr(chBuffer, deviceOrder[j][k]))
					{
						continue;
					}
				}

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
		if (!InitCL())
			FatalError(__FILE__, __LINE__, "Failed to initialize OpenCL");
		m_Initialized = true;
	}
	size_t size;
	cl_int error;
	char *source = loadSource(file, &size);
	m_Program = clCreateProgramWithSource(m_Context, 1, (const char **)&source, &size, &error);
	CheckCL(error, __FILE__, __LINE__);
	error = clBuildProgram(m_Program, 0, nullptr,
						   "-cl-fast-relaxed-math -cl-mad-enable "
						   "-cl-denorms-are-zero -cl-no-signed-zeros "
						   "-cl-unsafe-math-optimizations -cl-finite-math-only "
						   "-cl-strict-aliasing",
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

	m_WorkSize = new size_t[3];
	m_WorkSize[0] = std::get<0>(workSize);
	m_WorkSize[1] = std::get<1>(workSize);
	m_WorkSize[2] = std::get<2>(workSize);
}

Kernel::~Kernel()
{
	if (m_Kernel)
		clReleaseKernel(m_Kernel);
	if (m_Program)
		clReleaseProgram(m_Program);

	delete[] m_WorkSize;
}

bool Kernel::InitCL()
{
	cl_platform_id platform;
	cl_device_id *devices;
	cl_uint devCount;
	cl_int error;

	if (!CheckCL(error = getPlatformID(&platform), __FILE__, __LINE__))
		return false;
#ifdef __APPLE__
	if (!CheckCL(error = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, nullptr, &devCount), __FILE__, __LINE__))
		return false;
	devices = new cl_device_id[devCount];
	if (!CheckCL(error = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, devCount, devices, nullptr), __FILE__, __LINE__))
		return false;

	if (devCount >= 2)
	{
		// swap devices so dGPU gets selected
		std::swap(devices[0], devices[1]);
	}
#else
	if (!CheckCL(error = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, nullptr, &devCount), __FILE__, __LINE__))
		return false;
	devices = new cl_device_id[devCount];
	if (!CheckCL(error = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, devCount, devices, nullptr), __FILE__, __LINE__))
		return false;
#endif
	uint deviceUsed = 0;
	uint endDev = devCount;
	bool canShare = false;

	for (uint i = deviceUsed; (!canShare && (i < endDev)); ++i)
	{
		size_t extensionSize;
		CheckCL(error = clGetDeviceInfo(devices[i], CL_DEVICE_EXTENSIONS, 0, nullptr, &extensionSize), __FILE__,
				__LINE__);
		if (extensionSize > 0)
		{
			char *extensions = (char *)malloc(extensionSize);
			CheckCL(error =
						clGetDeviceInfo(devices[i], CL_DEVICE_EXTENSIONS, extensionSize, extensions, &extensionSize),
					__FILE__, __LINE__);
			std::string devices(extensions);
			std::transform(devices.begin(), devices.end(), devices.begin(), ::tolower);
			free(extensions);
			size_t oldPos = 0;
			size_t spacePos = devices.find(' ', oldPos); // extensions string is space delimited

#ifdef __APPLE__
			const char *neededProp = "cl_apple_gl_sharing";
#else
			const char *neededProp = "cl_khr_gl_sharing";
#endif

			while (spacePos != devices.npos)
			{
				if (strcmp(neededProp, devices.substr(oldPos, spacePos - oldPos).c_str()) == 0)
				{
					canShare = true; // device supports context sharing with OpenGL
					deviceUsed = i;
					break;
				}
				do
				{
					oldPos = spacePos + 1;
					spacePos = devices.find(' ', oldPos);
				} while (spacePos == oldPos);
			}
		}
	}

	if (canShare)
	{
		std::cout << "Using CL-GL Interop" << std::endl;
	}
	else
	{
		std::cout << "No device found that supports CL/GL context sharing" << std::endl;
		return false;
	}

#ifdef _WIN32
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

	// attempt to create a context with the requested features
	canDoInterop = true;
	m_Context = clCreateContext(props, 1, &devices[deviceUsed], nullptr, nullptr, &error);
	if (error != 0)
	{
		// that didn't work, let's take what we can get
		cl_context_properties props[] = {0};
		m_Context = clCreateContext(props, 1, &devices[deviceUsed], nullptr, nullptr, &error);
		canDoInterop = false;
	}

	m_Device = getFirstDevice(m_Context);
	if (!CheckCL(error, __FILE__, __LINE__))
	{
		return false;
	}

	// print device name
	std::vector<char> device_string(1024);
	clGetDeviceInfo(devices[deviceUsed], CL_DEVICE_NAME, device_string.size(), device_string.data(), nullptr);
	std::cout << "Device #" << deviceUsed << ", " << device_string.data() << std::endl;

	size_t p_size;
	size_t l_size;
	size_t d_size;
	clGetDeviceInfo(devices[deviceUsed], CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &p_size, nullptr);
	clGetDeviceInfo(devices[deviceUsed], CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(size_t), &l_size, nullptr);
	clGetDeviceInfo(devices[deviceUsed], CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(size_t), &d_size, nullptr);
	std::cout << "\tMax Work Group Size: " << p_size << std::endl;
	std::cout << "\tMax Work Item Sizes: " << l_size << std::endl;
	std::cout << "\tMax Work Item Dimensions: " << d_size << std::endl;

	// create a command-queue
	m_Queue = clCreateCommandQueue(m_Context, devices[deviceUsed], 0, &error);

	bool result = CheckCL(error, __FILE__, __LINE__);
	Kernel::m_Initialized = result;
	return result;
}

cl_int Kernel::run()
{
	glFinish();
	return clEnqueueNDRangeKernel(m_Queue, m_Kernel, 2, 0, m_WorkSize, 0, 0, 0, 0);
}

cl_int Kernel::run(cl_mem *buffers, int count)
{
	cl_int err = CL_SUCCESS;

	glFinish();
	if (Kernel::canDoInterop)
	{
		err = clEnqueueAcquireGLObjects(m_Queue, count, buffers, 0, 0, 0);
		err = clEnqueueNDRangeKernel(m_Queue, m_Kernel, 2, nullptr, m_WorkSize, 0, 0, 0, 0);
		err = clEnqueueReleaseGLObjects(m_Queue, count, buffers, 0, 0, 0);
	}
	else
	{
		err = clEnqueueNDRangeKernel(m_Queue, m_Kernel, 2, nullptr, m_WorkSize, 0, 0, 0, 0);
	}
	return err;
}

cl_int Kernel::run(TextureBuffer *buffer)
{
	cl_int err = CL_SUCCESS;
	glFinish();
	if (Kernel::canDoInterop)
	{
		err = clEnqueueAcquireGLObjects(m_Queue, 1, buffer->getDevicePtr(), 0, 0, 0);
		err = clEnqueueNDRangeKernel(m_Queue, m_Kernel, 2, nullptr, m_WorkSize, 0, 0, 0, 0);
		err = clEnqueueReleaseGLObjects(m_Queue, 1, buffer->getDevicePtr(), 0, 0, 0);
	}
	else
	{
		err = clEnqueueNDRangeKernel(m_Queue, m_Kernel, 2, nullptr, m_WorkSize, 0, 0, 0, 0);
	}

	return err;
}

cl_int Kernel::run(size_t count)
{
	cl_int err = CL_SUCCESS;
	err = clEnqueueNDRangeKernel(m_Queue, m_Kernel, 1, 0, &count, 0, 0, 0, 0);
	return err;
}

cl_int Kernel::syncQueue() { return clFinish(m_Queue); }
} // namespace cl