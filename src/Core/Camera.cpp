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
	calcForward();
	position = pos;
	aspectRatio = planeWidth / planeHeight;
	fovDistance = tanf(glm::radians(fov * 0.5f));
	isDirty = true;
}

Camera::~Camera() { delete gpuBuffer; }

Ray Camera::generateRay(float x, float y) const
{
	const vec3 &w = forward;
	const vec3 u = normalize(cross(w, up));
	const vec3 v = normalize(cross(u, w));

	const vec3 horizontal = u * fovDistance * aspectRatio;
	const vec3 vertical = v * fovDistance;

	const float PixelX = x * m_InvWidth;
	const float PixelY = y * m_InvHeight;

	const float ScreenX = 2.f * PixelX - 1.f;
	const float ScreenY = 1.f - 2.f * PixelY;

	const vec3 pointAtDistanceOneFromPlane = w + horizontal * ScreenX + vertical * ScreenY;

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
		calcForward();
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
	pitch = clamp(pitch + offset.y , -radians(85.f), radians(85.f));
	calcForward();
	isDirty = true;
}

void Camera::updateFOV(float offset) noexcept
{
	const float fov = clamp(FOV.x + offset, 20.0f, 160.0f);
	FOV = {fov, fov};
	fovDistance = tanf(glm::radians(fov * 0.5f));
	isDirty = true;
}

void Camera::calcForward() noexcept
{
	this->forward = {sinf(yaw) * cosf(pitch), sinf(pitch), -1.0f * cosf(yaw) * cosf(pitch)};
	this->right = cross(normalize(this->forward), this->up);
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
		if (gpuBuffer == nullptr)
			gpuBuffer = new Buffer(sizeof(CLCamera));

		auto *pointer = gpuBuffer->GetHostPtr<CLCamera>();

		const glm::vec3 u = normalize(cross(forward, vec3(0, 1, 0)));
		const glm::vec3 v = normalize(cross(u, forward));

		pointer->origin = vec4(position, 0.0f);
		pointer->viewDirection = vec4(forward, 0.0f);
		pointer->vertical = vec4(v * fovDistance, 0.0f);
		pointer->horizontal = vec4(u * fovDistance * aspectRatio, 0.0f);
		pointer->viewDistance = fovDistance;
		pointer->invWidth = m_InvWidth;
		pointer->invHeight = m_InvHeight;
		pointer->aspectRatio = aspectRatio;
		gpuBuffer->CopyToDevice(false);
	}

	isDirty = false;
}

std::future<void> Camera::updateGPUBufferAsync()
{
	return std::async([this]() -> void { updateGPUBuffer(); });
}
cl::Buffer *Camera::getGPUBuffer() { return gpuBuffer; }

} // namespace core