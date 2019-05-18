#include "Device.h"

namespace cl
{
Device::Device(cl_device_id id) : m_ID(id) {}

bool Device::isAvailable() const
{
	cl_bool deviceAvailable;
	clGetDeviceInfo(m_ID, CL_DEVICE_AVAILABLE, sizeof(cl_bool), &deviceAvailable, nullptr);
	return deviceAvailable;
}

std::string Device::getName() const
{
	std::vector<char> deviceName(512);
	clGetDeviceInfo(m_ID, CL_DEVICE_NAME, deviceName.size(), deviceName.data(), nullptr);
	return std::string(deviceName.data());
}

size_t Device::getGlobalMemorySize() const
{
	cl_ulong globalMemSize;
	clGetDeviceInfo(m_ID, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(globalMemSize), &globalMemSize, nullptr);
	return globalMemSize;
}

size_t Device::getLocalMemorySize() const
{
	cl_ulong localMemSize;
	clGetDeviceInfo(m_ID, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(localMemSize), &localMemSize, nullptr);
	return localMemSize;
}
unsigned int Device::getMaxFrequency() const
{
	cl_uint maxFrequency;
	clGetDeviceInfo(m_ID, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(maxFrequency), &maxFrequency, nullptr);
	return maxFrequency;
}

unsigned int Device::getComputeUnits() const
{
	cl_uint maxComputeUnits;
	clGetDeviceInfo(m_ID, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(maxComputeUnits), &maxComputeUnits, nullptr);
	return maxComputeUnits;
}

Device::Type Device::getType() const
{
	cl_device_type type;
	clGetDeviceInfo(m_ID, CL_DEVICE_TYPE, sizeof(cl_device_type), &type, nullptr);

	switch (type)
	{
	case (CL_DEVICE_TYPE_DEFAULT):
		return Type::Default;
	case (CL_DEVICE_TYPE_CPU):
		return Type::CPU;
	case (CL_DEVICE_TYPE_GPU):
		return Type::GPU;
	case (CL_DEVICE_TYPE_ACCELERATOR):
		return Type::Accelerator;
	case (CL_DEVICE_TYPE_CUSTOM):
		return Type::Custom;
	case (CL_DEVICE_TYPE_ALL):
		return Type::All;
	default:
		return Type::Unknown;
	}
}

size_t Device::getMaxWorkGroupSize() const
{
	size_t p_size;
	clGetDeviceInfo(m_ID, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &p_size, nullptr);
	return p_size;
}

size_t Device::getMaxWorkItemSizes() const
{
	size_t l_size;
	clGetDeviceInfo(m_ID, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(size_t), &l_size, nullptr);
	return l_size;
}

size_t Device::getMaxWorkDimensions() const
{
	size_t d_size;
	clGetDeviceInfo(m_ID, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(size_t), &d_size, nullptr);
	return d_size;
}

} // namespace cl
