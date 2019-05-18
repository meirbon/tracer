#pragma once

#include <OpenCL/cl.h>

#include <string>
#include <vector>

namespace cl
{
class Device
{
  public:
	enum Type
	{
		Default = CL_DEVICE_TYPE_DEFAULT,
		CPU = CL_DEVICE_TYPE_CPU,
		GPU = CL_DEVICE_TYPE_GPU,
		Accelerator = CL_DEVICE_TYPE_ACCELERATOR,
		Custom = CL_DEVICE_TYPE_CUSTOM,
		All = CL_DEVICE_TYPE_ALL,
		Unknown
	};

	Device() = default;
	Device(cl_device_id id);

	cl_device_id getID() const { return m_ID; }

	bool isAvailable() const;
	std::string getName() const;

	size_t getGlobalMemorySize() const;
	size_t getLocalMemorySize() const;

	unsigned int getMaxFrequency() const;
	unsigned int getComputeUnits() const;

	Type getType() const;

	size_t getMaxWorkGroupSize() const;
	size_t getMaxWorkItemSizes() const;
	size_t getMaxWorkDimensions() const;

	template <typename T> void getInfo(cl_device_info info, T *data)
	{
		clGetDeviceInfo(m_ID, info, sizeof(T), data, nullptr);
	}

	void getInfo(cl_device_info info, std::vector<char> *data)
	{
		clGetDeviceInfo(m_ID, info, sizeof(char) * (data->size()), data->data(), nullptr);
	}

  private:
	cl_device_id m_ID;
};

}