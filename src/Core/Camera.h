#pragma once

#include <future>

#include <CL/Buffer.h>
#include <Core/Ray.h>
#include <Utils/RandomGenerator.h>
#include <glm/glm.hpp>

namespace core
{

struct Camera
{
	struct CLCamera
	{
		glm::vec4 origin;		 // 16
		glm::vec4 viewDirection; // 32
		glm::vec4 vertical;
		glm::vec4 horizontal;

		float viewDistance; // 36
		float invWidth;		// 40
		float invHeight;	// 44
		float aspectRatio;  // 48
	};

	explicit Camera(int width, int height, float fov, float mouseSens = 0.005f,
					glm::vec3 pos = glm::vec3(0.f, 0.f, 1.f));
	~Camera();

	Ray generateRay(float x, float y) const;

	Ray generateRandomRay(float x, float y, RandomGenerator &rng) const;

	void processMouse(glm::vec2 xy) noexcept;

	void move(glm::vec3 offset) noexcept;

	void rotate(glm::vec2 offset) noexcept;

	void updateFOV(float offset) noexcept;

	void updateVectors() noexcept;

	void setWidth(int width);

	void setHeight(int height);

	void setDimensions(glm::ivec2 dims);

	cl::Buffer<CLCamera> *getGPUBuffer();
	void updateGPUBuffer();
	std::future<void> updateGPUBufferAsync();

	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 forward;
	glm::vec3 position;

	struct
	{
		float x;
		float y;
	} FOV = {70.f, 70.f};

	float fovDistance;
	float rotationSpeed = 0.005f;
	float aspectRatio = 0;
	float yaw = 0;
	float pitch = 0;
	float planeWidth, planeHeight;
	float m_InvWidth, m_InvHeight;

	bool isDirty = false;

  private:
	cl::Buffer<CLCamera> *gpuBuffer = nullptr;
};
} // namespace core
