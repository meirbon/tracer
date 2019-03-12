#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "GpuTriangleList.h"
#include "SceneObjectList.h"

namespace model
{
void Load(const std::string &inputFile, uint matIndex, glm::vec3 translation,
          float scale, SceneObjectList *objectList,
          glm::mat4 transform = glm::mat4(1.0f));

void Load(const std::string &inputFile, uint matIndex, glm::vec3 translation,
          float scale, GpuTriangleList *objectList,
          glm::mat4 transform = glm::mat4(1.0f));
}; // namespace model
