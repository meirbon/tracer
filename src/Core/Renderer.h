#pragma once

#include "Core/Ray.h"
#include "Core/Surface.h"
#include "Utils/RandomGenerator.h"

#include "GL/Texture.h"

namespace core
{
enum Mode
{
	Reference = 0,
	NEE = 1,
	IS = 2,
	NEE_IS = 3,
	NEE_MIS = 4,
	ReferenceMicrofacet = 5,
	NEEMicrofacet = 6,
};

class Renderer
{
  public:
	virtual void Render(Surface *output) = 0;

	inline virtual void Reset() {}

	inline virtual int GetSamples() const { return 0; }

	virtual void Resize(gl::Texture *newOutput) = 0;

	virtual ~Renderer() = default;

	virtual void SwitchSkybox() = 0;

	inline const Mode &GetMode() const noexcept { return m_Mode; }

	inline virtual const char *GetModeString() const noexcept
	{
		switch (m_Mode)
		{
		case (NEE):
			return "NEE";
		case (IS):
			return "IS";
		case (NEE_IS):
			return "NEE IS";
		case (NEE_MIS):
			return "NEE MIS";
		case (ReferenceMicrofacet):
			return "Ref MF";
		case (NEEMicrofacet):
			return "NEE MF";
		case (Reference):
		default:
			return "Ref";
		}
	}

	virtual void SetMode(Mode mode) { m_Mode = mode; }

	virtual void SetMode(std::string){};
	virtual const std::vector<const char*> &GetModes() { return modes; };

  protected:
	Mode m_Mode = Reference;
	std::vector<const char*> modes;
};
} // namespace core