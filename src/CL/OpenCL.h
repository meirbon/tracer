#pragma once

#include <algorithm>
#include <iostream>
#include <tuple>

#ifdef __APPLE__
#define CL_SILENCE_DEPRECATION
#include <GL/glew.h>
#include <OpenCL/cl.h>
#include <OpenCL/cl_gl_ext.h>
#include <OpenGL/CGLCurrent.h>
#include <OpenGL/CGLDevice.h>
#else
#include <GL/glew.h>
#ifdef __linux__
#include <GL/glx.h>
#endif
#include <CL/cl.h>
#include <CL/cl_gl_ext.h>
#endif

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include "Utils/Messages.h"

namespace cl
{
inline bool CheckCL(cl_int result, const char *file, int line)
{
	using namespace utils;

	switch (result)
	{
	case (CL_SUCCESS):
		return true;
	case (CL_DEVICE_NOT_FOUND):
		FatalError(file, line, "Error: CL_DEVICE_NOT_FOUND", "OpenCL error");
		break;
	case (CL_DEVICE_NOT_AVAILABLE):
		FatalError(file, line, "Error: CL_DEVICE_NOT_AVAILABLE", "OpenCL error");
		break;
	case (CL_COMPILER_NOT_AVAILABLE):
		FatalError(file, line, "Error: CL_COMPILER_NOT_AVAILABLE", "OpenCL error");
		break;
	case (CL_MEM_OBJECT_ALLOCATION_FAILURE):
		FatalError(file, line, "Error: CL_MEM_OBJECT_ALLOCATION_FAILURE", "OpenCL error");
		break;
	case (CL_OUT_OF_RESOURCES):
		FatalError(file, line, "Error: CL_OUT_OF_RESOURCES", "OpenCL error");
		break;
	case (CL_OUT_OF_HOST_MEMORY):
		FatalError(file, line, "Error: CL_OUT_OF_HOST_MEMORY", "OpenCL error");
		break;
	case (CL_PROFILING_INFO_NOT_AVAILABLE):
		FatalError(file, line, "Error: CL_PROFILING_INFO_NOT_AVAILABLE", "OpenCL error");
		break;
	case (CL_MEM_COPY_OVERLAP):
		FatalError(file, line, "Error: CL_MEM_COPY_OVERLAP", "OpenCL error");
		break;
	case (CL_IMAGE_FORMAT_MISMATCH):
		FatalError(file, line, "Error: CL_IMAGE_FORMAT_MISMATCH", "OpenCL error");
		break;
	case (CL_IMAGE_FORMAT_NOT_SUPPORTED):
		FatalError(file, line, "Error: CL_IMAGE_FORMAT_NOT_SUPPORTED", "OpenCL error");
		break;
	case (CL_BUILD_PROGRAM_FAILURE):
		FatalError(file, line, "Error: CL_BUILD_PROGRAM_FAILURE", "OpenCL error");
		break;
	case (CL_MAP_FAILURE):
		FatalError(file, line, "Error: CL_MAP_FAILURE", "OpenCL error");
		break;
	case (CL_MISALIGNED_SUB_BUFFER_OFFSET):
		FatalError(file, line, "Error: CL_MISALIGNED_SUB_BUFFER_OFFSET", "OpenCL error");
		break;
	case (CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST):
		FatalError(file, line, "Error: CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST", "OpenCL error");
		break;
	case (CL_INVALID_VALUE):
		FatalError(file, line, "Error: CL_INVALID_VALUE", "OpenCL error");
		break;
	case (CL_INVALID_DEVICE_TYPE):
		FatalError(file, line, "Error: CL_INVALID_DEVICE_TYPE", "OpenCL error");
		break;
	case (CL_INVALID_PLATFORM):
		FatalError(file, line, "Error: CL_INVALID_PLATFORM", "OpenCL error");
		break;
	case (CL_INVALID_DEVICE):
		FatalError(file, line, "Error: CL_INVALID_DEVICE", "OpenCL error");
		break;
	case (CL_INVALID_CONTEXT):
		FatalError(file, line, "Error: CL_INVALID_CONTEXT", "OpenCL error");
		break;
	case (CL_INVALID_QUEUE_PROPERTIES):
		FatalError(file, line, "Error: CL_INVALID_QUEUE_PROPERTIES", "OpenCL error");
		break;
	case (CL_INVALID_COMMAND_QUEUE):
		FatalError(file, line, "Error: CL_INVALID_COMMAND_QUEUE", "OpenCL error");
		break;
	case (CL_INVALID_HOST_PTR):
		FatalError(file, line, "Error: CL_INVALID_HOST_PTR", "OpenCL error");
		break;
	case (CL_INVALID_MEM_OBJECT):
		FatalError(file, line, "Error: CL_INVALID_MEM_OBJECT", "OpenCL error");
		break;
	case (CL_INVALID_IMAGE_FORMAT_DESCRIPTOR):
		FatalError(file, line, "Error: CL_INVALID_IMAGE_FORMAT_DESCRIPTOR", "OpenCL error");
		break;
	case (CL_INVALID_IMAGE_SIZE):
		FatalError(file, line, "Error: CL_INVALID_IMAGE_SIZE", "OpenCL error");
		break;
	case (CL_INVALID_SAMPLER):
		FatalError(file, line, "Error: CL_INVALID_SAMPLER", "OpenCL error");
		break;
	case (CL_INVALID_BINARY):
		FatalError(file, line, "Error: CL_INVALID_BINARY", "OpenCL error");
		break;
	case (CL_INVALID_BUILD_OPTIONS):
		FatalError(file, line, "Error: CL_INVALID_BUILD_OPTIONS", "OpenCL error");
		break;
	case (CL_INVALID_PROGRAM):
		FatalError(file, line, "Error: CL_INVALID_PROGRAM", "OpenCL error");
		break;
	case (CL_INVALID_PROGRAM_EXECUTABLE):
		FatalError(file, line, "Error: CL_INVALID_PROGRAM_EXECUTABLE", "OpenCL error");
		break;
	case (CL_INVALID_KERNEL_NAME):
		FatalError(file, line, "Error: CL_INVALID_KERNEL_NAME", "OpenCL error");
		break;
	case (CL_INVALID_KERNEL_DEFINITION):
		FatalError(file, line, "Error: CL_INVALID_KERNEL_DEFINITION", "OpenCL error");
		break;
	case (CL_INVALID_KERNEL):
		FatalError(file, line, "Error: CL_INVALID_KERNEL", "OpenCL error");
		break;
	case (CL_INVALID_ARG_INDEX):
		FatalError(file, line, "Error: CL_INVALID_ARG_INDEX", "OpenCL error");
		break;
	case (CL_INVALID_ARG_VALUE):
		FatalError(file, line, "Error: CL_INVALID_ARG_VALUE", "OpenCL error");
		break;
	case (CL_INVALID_ARG_SIZE):
		FatalError(file, line, "Error: CL_INVALID_ARG_SIZE", "OpenCL error");
		break;
	case (CL_INVALID_KERNEL_ARGS):
		FatalError(file, line, "Error: CL_INVALID_KERNEL_ARGS", "OpenCL error");
		break;
	case (CL_INVALID_WORK_DIMENSION):
		FatalError(file, line, "Error: CL_INVALID_WORK_DIMENSION", "OpenCL error");
		break;
	case (CL_INVALID_WORK_GROUP_SIZE):
		FatalError(file, line, "Error: CL_INVALID_WORK_GROUP_SIZE", "OpenCL error");
		break;
	case (CL_INVALID_WORK_ITEM_SIZE):
		FatalError(file, line, "Error: CL_INVALID_WORK_ITEM_SIZE", "OpenCL error");
		break;
	case (CL_INVALID_GLOBAL_OFFSET):
		FatalError(file, line, "Error: CL_INVALID_GLOBAL_OFFSET", "OpenCL error");
		break;
	case (CL_INVALID_EVENT_WAIT_LIST):
		FatalError(file, line, "Error: CL_INVALID_EVENT_WAIT_LIST", "OpenCL error");
		break;
	case (CL_INVALID_EVENT):
		FatalError(file, line, "Error: CL_INVALID_EVENT", "OpenCL error");
		break;
	case (CL_INVALID_OPERATION):
		FatalError(file, line, "Error: CL_INVALID_OPERATION", "OpenCL error");
		break;
	case (CL_INVALID_GL_OBJECT):
		FatalError(file, line, "Error: CL_INVALID_GL_OBJECT", "OpenCL error");
		break;
	case (CL_INVALID_BUFFER_SIZE):
		FatalError(file, line, "Error: CL_INVALID_BUFFER_SIZE", "OpenCL error");
		break;
	case (CL_INVALID_MIP_LEVEL):
		FatalError(file, line, "Error: CL_INVALID_MIP_LEVEL", "OpenCL error");
		break;
	case (CL_INVALID_GLOBAL_WORK_SIZE):
		FatalError(file, line, "Error: CL_INVALID_GLOBAL_WORK_SIZE", "OpenCL error");
		break;
	default:
		return false;
	}

	return false;
}

#define CHECKCL(x) CheckCL(x, __FILE__, __LINE__)

class Kernel
{
  public:
	Kernel() = default;
	Kernel(const char *file, const char *entryPoint, std::tuple<size_t, size_t, size_t> workSize);
	~Kernel();

	cl_kernel &getKernel() { return m_Kernel; }

	static cl_command_queue &getQueue() { return m_Queue; }

	static cl_context &getContext() { return m_Context; }

	static cl_device_id &getDevice() { return m_Device; }

	// methods
	cl_int run();

	cl_int run(cl_mem *buffers, int count = 1);

	cl_int run(cl::TextureBuffer *buffer);

	cl_int run(size_t count);

	static cl_int syncQueue();

	cl_int setArgument(int idx, cl_mem *buffer) { return clSetKernelArg(m_Kernel, idx, sizeof(cl_mem), buffer); }

	template <typename Buffer> cl_int setArgument(int idx, Buffer *buffer)
	{
		return clSetKernelArg(m_Kernel, idx, sizeof(cl_mem), buffer->getDevicePtr());
	}

	template <typename T> cl_int setArgument(int idx, const T &data)
	{
		return clSetKernelArg(m_Kernel, idx, sizeof(T), (void *)&data);
	}

	inline void SetWorkSize(size_t x, size_t y, size_t z)
	{
		m_WorkSize[0] = x;
		m_WorkSize[1] = y;
		m_WorkSize[2] = z;
	}

	static bool InitCL();

  private:
	// data members
	cl_kernel m_Kernel;
	cl_program m_Program;
	static bool m_Initialized;
	static cl_device_id m_Device;
	static cl_context m_Context; // simplifies some things, but limits us to one device
	static cl_command_queue m_Queue;
	static char *m_Log;

	size_t *m_WorkSize;

  public:
	static bool canDoInterop;
};
} // namespace cl