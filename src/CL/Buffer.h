#pragma once

#include <algorithm>
#include <iostream>
#include <tuple>
#include <vector>

#ifdef __APPLE__
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

namespace cl
{
template <typename T> class Buffer;
class TextureBuffer;
} // namespace cl

#include "OpenCL.h"
#include <GL/Texture.h>

namespace gl
{
class Texture;
}

namespace cl
{

enum BufferType
{
	TEXTURE = 1,
	TARGET = 2
};

template <typename T> class Buffer
{
  public:
	enum Type
	{
		R = CL_MEM_READ_ONLY,
		W = CL_MEM_WRITE_ONLY,
		RW = CL_MEM_READ_WRITE
	};

	Buffer<T>(unsigned int count, Type type = RW)
	{
		count = glm::max(static_cast<unsigned int>(1), count);
		m_Size = count * sizeof(T);
		m_DeviceBuffer = clCreateBuffer(Kernel::getContext(), type, m_Size, 0, 0);
		m_HostBuffer.resize(count);
	}

	Buffer<T>(const std::vector<T> &data, Type type = RW)
	{
		m_Size = data.size() * sizeof(T);
		m_DeviceBuffer = clCreateBuffer(Kernel::getContext(), type, m_Size, 0, 0);
		m_HostBuffer.resize(data.size());
		memcpy(m_HostBuffer.data(), data.data(), data.size() * sizeof(T));
	}

	Buffer<T>(const T *data, Type type = RW)
	{
		m_Size = 1 * sizeof(T);
		m_DeviceBuffer = clCreateBuffer(Kernel::getContext(), type, m_Size, 0, 0);
		m_HostBuffer.resize(1);
		m_HostBuffer[0] = *data;
	}

	~Buffer()
	{
		if (m_DeviceBuffer)
			clReleaseMemObject(m_DeviceBuffer);
	}

	const cl_mem *getDevicePtr() const { return &m_DeviceBuffer; }

	std::vector<T> &getHostPtr() { return m_HostBuffer; }

	cl_int copyToDevice(bool blocking = true)
	{
		return clEnqueueWriteBuffer(Kernel::getQueue(), m_DeviceBuffer, blocking, 0, m_Size,
									(const void *)m_HostBuffer.data(), 0, 0, 0);
	}

	cl_int copyFromDevice(bool blocking = true)
	{
		clEnqueueReadBuffer(Kernel::getQueue(), m_DeviceBuffer, blocking, 0, m_Size, m_HostBuffer.data(), 0, 0, 0);
	}

	cl_int copyTo(Buffer *buffer)
	{
		clEnqueueCopyBuffer(Kernel::getQueue(), m_DeviceBuffer, buffer->m_DeviceBuffer, 0, 0, m_Size, 0, 0, 0);
	}

	cl_int clear()
	{
		unsigned int value = 0;
		return clEnqueueFillBuffer(Kernel::getQueue(), m_DeviceBuffer, &value, sizeof(int), 0, m_Size, 0, nullptr, nullptr);
	}

	std::vector<T> read()
	{
		auto *data = (unsigned char *)clEnqueueMapBuffer(Kernel::getQueue(), m_PinnedBuffer, CL_TRUE, CL_MAP_READ, 0,
														 m_Size, 0, nullptr, nullptr, nullptr);
		clEnqueueReadBuffer(Kernel::getQueue(), m_DeviceBuffer, CL_TRUE, 0, m_Size, data, 0, nullptr, nullptr);
		std::vector<T> d(m_Size);
		memcpy(d.data(), data, m_Size);
		return d;
	}

	cl_int write(T *data)
	{
		auto *mappedData = (T *)clEnqueueMapBuffer(Kernel::getQueue(), m_PinnedBuffer, CL_TRUE, CL_MAP_WRITE, 0, m_Size,
												   0, nullptr, nullptr, nullptr);

		memcpy(mappedData, data, m_Size);
		return clEnqueueWriteBuffer(Kernel::getQueue(), m_DeviceBuffer, CL_FALSE, 0, m_Size, data, 0, nullptr, nullptr);
	}

  private:
	cl_mem m_DeviceBuffer = 0, m_PinnedBuffer = 0;
	std::vector<T> m_HostBuffer;
	size_t m_Size;
};

class TextureBuffer
{
  public:
	enum Type
	{
		Texture,
		Target
	};

	TextureBuffer() = default;
	TextureBuffer(gl::Texture *texture, cl::BufferType t);

	~TextureBuffer()
	{
		if (m_DeviceBuffer)
			clReleaseMemObject(m_DeviceBuffer);
	}

	const cl_mem *getDevicePtr() const { return &m_DeviceBuffer; }

	cl_int clear();

	std::vector<float> read();

	cl_int write(float *data);

  private:
	// data members
	cl_mem m_DeviceBuffer = 0, m_PinnedBuffer = 0;
	unsigned int m_TextureId;
	size_t m_Size = 0;
	std::vector<unsigned int> m_HostBuffer;

  private:
};
} // namespace cl
