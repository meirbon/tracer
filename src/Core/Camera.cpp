#include "Camera.h"

using namespace glm;

namespace core
{
Camera::Camera(int width, int height, float fov, float mouseSens, vec3 pos)
	: position(pos), yaw(0), pitch(0), planeWidth((float)width), planeHeight((float)height), m_InvWidth(1.f / width),
	  m_InvHeight(1.f / height)
{
	up = vec3(0, 1, 0);
	right = vec3(1, 0, 0);
	forward = cross(up, right);

	rotationSpeed = mouseSens;
	updateVectors();
	position = pos;
	aspectRatio = planeWidth / planeHeight;
	fovDistance = tanf(glm::radians(fov * 0.5f));
	isDirty = true;
}

Camera::~Camera() { delete m_GPUBuffer; }

Ray Camera::generateRay(float x, float y) const
{
	const vec3 horizontal = right * fovDistance * aspectRatio;
	const vec3 vertical = up * fovDistance;

	const float PixelX = x * m_InvWidth;
	const float PixelY = y * m_InvHeight;

	const float ScreenX = 2.f * PixelX - 1.f;
	const float ScreenY = 1.f - 2.f * PixelY;

	const vec3 pointAtDistanceOneFromPlane = forward + horizontal * ScreenX + vertical * ScreenY;

	return {position, normalize(pointAtDistanceOneFromPlane)};
}

Ray Camera::generateRandomRay(float x, float y, RandomGenerator &rng) const
{
	const float newX = x + rng.Rand(1.f) - .5f;
	const float newY = y + rng.Rand(1.f) - .5f;

	return generateRay(newX, newY);
}

void Camera::processMouse(glm::vec2 xy) noexcept
{
	if (any(notEqual(xy, vec2(0, 0))))
	{
		rotate(xy * rotationSpeed);
		updateVectors();
		isDirty = true;
	}
}

void Camera::move(glm::vec3 offset) noexcept
{
	position += up * offset.y + offset.x * normalize(cross(up, forward)) + forward * offset.z;
	isDirty = true;
}

void Camera::rotate(glm::vec2 offset) noexcept
{
	yaw += offset.x;
	pitch = clamp(pitch + offset.y, -radians(85.f), radians(85.f));
	updateVectors();
	isDirty = true;
}

void Camera::updateFOV(float offset) noexcept
{
	const float fov = clamp(FOV.x + offset, 20.0f, 160.0f);
	FOV = {fov, fov};
	fovDistance = tanf(glm::radians(fov * 0.5f));
	isDirty = true;
}

void Camera::updateVectors() noexcept
{
	forward = {sinf(yaw) * cosf(pitch), sinf(pitch), -1.0f * cosf(yaw) * cosf(pitch)};
	vec3 temp = vec3(0, 1, 0);
	if (forward.y >= 0.99f)
		temp = vec3(0, 0, 1);
	right = normalize(cross(normalize(forward), temp));
	up = -normalize(cross(forward, right));
	isDirty = true;
}

void Camera::setWidth(int width)
{
	planeWidth = float(width);
	m_InvWidth = 1.0f / planeWidth;
	aspectRatio = float(planeWidth) / planeHeight;
	isDirty = true;
}

void Camera::setHeight(int height)
{
	planeHeight = float(height);
	m_InvHeight = (float)1.0f / float(height);
	aspectRatio = float(planeWidth) / planeHeight;
	isDirty = true;
}

void Camera::setDimensions(glm::ivec2 dims)
{
	planeWidth = float(dims.x);
	planeHeight = float(dims.y);

	m_InvWidth = 1.0f / planeWidth;
	m_InvHeight = 1.0f / planeHeight;
	aspectRatio = planeWidth / planeHeight;
	isDirty = true;
}

void Camera::updateGPUBuffer()
{
	using namespace cl;
	if (isDirty)
	{
		if (!m_GPUBuffer)
			m_GPUBuffer = new Buffer<CLCamera>(1);

		auto &gpuCamera = m_GPUBuffer->getHostPtr()[0];

		const glm::vec3 u = normalize(cross(forward, vec3(0, 1, 0)));
		const glm::vec3 v = normalize(cross(u, forward));

		gpuCamera.origin = vec4(position, 0.0f);
		gpuCamera.viewDirection = vec4(forward, 0.0f);
		gpuCamera.vertical = vec4(v * fovDistance, 0.0f);
		gpuCamera.horizontal = vec4(u * fovDistance * aspectRatio, 0.0f);
		gpuCamera.viewDistance = fovDistance;
		gpuCamera.invWidth = m_InvWidth;
		gpuCamera.invHeight = m_InvHeight;
		gpuCamera.aspectRatio = aspectRatio;
		m_GPUBuffer->copyToDevice(false);
	}

	isDirty = false;
}

std::future<void> Camera::updateGPUBufferAsync()
{
	return std::async([this]() -> void { updateGPUBuffer(); });
}

cl::Buffer<Camera::CLCamera> *Camera::getGPUBuffer() { return m_GPUBuffer; }

} // namespace core