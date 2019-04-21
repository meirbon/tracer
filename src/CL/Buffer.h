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

namespace gl
{
class Texture;
}

namespace cl
{
enum BufferType
{
  	DEFAULT = 0,
	TEXTURE = 1,
	TARGET = 2
};

class Buffer
{
  public:
	// constructor / destructor
	Buffer() : hostBuffer(0) {}

	Buffer(unsigned int size, void *ptr = 0);
	Buffer(gl::Texture *texture, BufferType t = TARGET);

	~Buffer();

	cl_mem *GetDevicePtr() { return &deviceBuffer; }

	template <class T> inline T *GetHostPtr() { return (T *)hostBuffer; }

	void CopyToDevice(bool blocking = true);

	void CopyFromDevice(bool blocking = true);

	void CopyTo(Buffer *buffer);

	void Clear();

	void Read(void *dst);

	void Write(void *dst);

	// data members
	cl_mem deviceBuffer, pinnedBuffer;
	unsigned int *hostBuffer;
	unsigned int m_Type, m_Size, m_TextureID;
	unsigned int sizeInQuads;
	bool ownData;
};
} // namespace cl
