#include "Buffer.h"
#include "OpenCL.h"

cl::TextureBuffer::TextureBuffer(gl::Texture *texture, cl::BufferType t) : m_PinnedBuffer(0)
{
	m_TextureId = texture->GetID();
	m_Size = texture->GetWidth() * texture->GetHeight() * sizeof(unsigned int);
	cl_mem_flags flags = t == TARGET ? CL_MEM_WRITE_ONLY : CL_MEM_READ_ONLY;

	if (Kernel::canDoInterop)
	{
		m_DeviceBuffer = clCreateFromGLTexture(Kernel::getContext(), flags, GL_TEXTURE_2D, 0, m_TextureId, 0);
	}
	else
	{
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

		m_HostBuffer.resize(m_Size);
		m_DeviceBuffer = clCreateImage(Kernel::getContext(), flags, &format, &desc, m_HostBuffer.data(), 0);
	}
}

cl_int cl::TextureBuffer::clear()
{
	unsigned int value = 0;
	return clEnqueueFillBuffer(Kernel::getQueue(), m_DeviceBuffer, &value, 4, 0, m_Size, 0, nullptr, nullptr);
}

std::vector<float> cl::TextureBuffer::read()
{
	auto *data = (unsigned char *)clEnqueueMapBuffer(Kernel::getQueue(), m_PinnedBuffer, CL_TRUE, CL_MAP_READ, 0,
													 m_Size, 0, nullptr, nullptr, nullptr);
	clEnqueueReadBuffer(Kernel::getQueue(), m_DeviceBuffer, CL_TRUE, 0, m_Size, data, 0, nullptr, nullptr);
	std::vector<float> d(m_Size);
	memcpy(d.data(), data, m_Size);
	return d;
}

cl_int cl::TextureBuffer::write(float *data)
{
	auto *mappedData = (float *)clEnqueueMapBuffer(Kernel::getQueue(), m_PinnedBuffer, CL_TRUE, CL_MAP_WRITE, 0, m_Size,
												   0, nullptr, nullptr, nullptr);

	memcpy(data, mappedData, m_Size);
	return clEnqueueWriteBuffer(Kernel::getQueue(), m_DeviceBuffer, CL_FALSE, 0, m_Size, data, 0, nullptr, nullptr);
}
