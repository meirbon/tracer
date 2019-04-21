#include "CL/Buffer.h"

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include "CL/OpenCL.h"
#include "GL/Texture.h"
#include "Utils/Messages.h"

namespace cl
{

void Buffer::Write(void *dst)
{
	auto *data = (unsigned char *)clEnqueueMapBuffer(Kernel::GetQueue(), pinnedBuffer, CL_TRUE, CL_MAP_WRITE, 0, m_Size,
													 0, nullptr, nullptr, nullptr);
	memcpy(data, dst, m_Size);
	clEnqueueWriteBuffer(Kernel::GetQueue(), deviceBuffer, CL_FALSE, 0, m_Size, data, 0, nullptr, nullptr);
}

void Buffer::Read(void *dst)
{
	auto *data = (unsigned char *)clEnqueueMapBuffer(Kernel::GetQueue(), pinnedBuffer, CL_TRUE, CL_MAP_READ, 0, m_Size,
													 0, nullptr, nullptr, nullptr);
	clEnqueueReadBuffer(Kernel::GetQueue(), deviceBuffer, CL_TRUE, 0, m_Size, data, 0, nullptr, nullptr);
	memcpy(dst, data, m_Size);
}

Buffer::Buffer(unsigned int size, void *ptr)
{
	ownData = false;
	m_Size = size;
	sizeInQuads = size / 4;
	m_TextureID = 0; // not representing a texture
	deviceBuffer = clCreateBuffer(Kernel::GetContext(), CL_MEM_READ_WRITE, m_Size, 0, 0);

	if (!ptr)
	{
		hostBuffer = new unsigned int[sizeInQuads];
		ownData = true;
	}
	else
	{
		hostBuffer = (unsigned int *)ptr;
	}
}

Buffer::Buffer(gl::Texture *texture, cl::BufferType t) : m_Type(DEFAULT)
{
	m_TextureID = texture->GetID(); // representing texture N
	m_Size = texture->GetWidth() * texture->GetHeight() * sizeof(unsigned int);
	sizeInQuads = m_Size / 4;
	ownData = true;
	if (Kernel::canDoInterop)
	{
#ifdef USE_CPU_DEVICE
		deviceBuffer = clCreateFromGLTexture2D(Kernel::GetContext(), CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, textureID, 0);
#else
		if (t == TARGET)
		{
			deviceBuffer =
				clCreateFromGLTexture(Kernel::GetContext(), CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, m_TextureID, 0);
		}
		else
		{
			deviceBuffer =
				clCreateFromGLTexture(Kernel::GetContext(), CL_MEM_READ_ONLY, GL_TEXTURE_2D, 0, m_TextureID, 0);
		}
#endif
		hostBuffer = nullptr;
	}
	else
	{
		// can't directly generate buffer from texture
		hostBuffer = new unsigned int[sizeInQuads];
		cl_image_format format;
		cl_image_desc desc;
		desc.image_type = CL_MEM_OBJECT_IMAGE2D;
		desc.image_width = texture->GetWidth();
		desc.image_height = texture->GetHeight();
		desc.image_depth = 0;
		desc.image_array_size = 0;
		desc.image_row_pitch = 0;
		desc.image_slice_pitch = 0;
		desc.num_mip_levels = 0;
		desc.num_samples = 0;
		desc.buffer = 0;
		format.image_channel_order = CL_RGBA;
		format.image_channel_data_type = CL_FLOAT;

		if (t == TARGET)
			deviceBuffer = clCreateImage(Kernel::GetContext(), CL_MEM_WRITE_ONLY, &format, &desc, hostBuffer, 0);
		else
			deviceBuffer = clCreateImage(Kernel::GetContext(), CL_MEM_READ_WRITE, &format, &desc, hostBuffer, 0);
	}
}

Buffer::~Buffer()
{
	if (ownData)
	{
		delete[] hostBuffer;
	}
}

void Buffer::CopyToDevice(bool blocking)
{
	CheckCL(clEnqueueWriteBuffer(Kernel::GetQueue(), deviceBuffer, blocking, 0, m_Size, hostBuffer, 0, 0, 0), __FILE__,
			__LINE__);
}

void Buffer::CopyFromDevice(bool blocking)
{
	CheckCL(clEnqueueReadBuffer(Kernel::GetQueue(), deviceBuffer, blocking, 0, m_Size, hostBuffer, 0, 0, 0), __FILE__,
			__LINE__);
}

void Buffer::CopyTo(Buffer *buffer)
{
	CheckCL(clEnqueueCopyBuffer(Kernel::GetQueue(), deviceBuffer, buffer->deviceBuffer, 0, 0, m_Size, 0, 0, 0),
			__FILE__, __LINE__);
}

void Buffer::Clear()
{
	unsigned int value = 0;
	CheckCL(clEnqueueFillBuffer(Kernel::GetQueue(), deviceBuffer, &value, 4, 0, m_Size, 0, nullptr, nullptr), __FILE__,
			__LINE__);
}
} // namespace cl