#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "SceneObjectList.h"

namespace prims
{
void Load(const std::string &inputFile, uint matIndex, glm::vec3 translation, float scale, SceneObjectList *objectList,
		  glm::mat4 transform = glm::mat4(1.0f));
}; // namespace prims
