#pragma once

#include "Materials/MaterialManager.h"

#include "Primitives/GpuTriangleList.h"
#include "Primitives/SceneObjectList.h"

void texturedScene(SceneObjectList *objectList);
void whittedScene(SceneObjectList *objectList);
void testScene(SceneObjectList *objectList);
void torusScene(SceneObjectList *objectList);
void teapotScene(SceneObjectList *objectList);
void NanoSuit(SceneObjectList *objectList);

// https://github.com/tiansijie/Tile_Based_WebGL_DeferredShader/tree/master/NPRChinesepainting/obj/cornell-box
void CornellBox(SceneObjectList *objectList);
void CBox(SceneObjectList *objectList);
void CBoxWater(SceneObjectList *objectList);
void Sponza(SceneObjectList *objectList);
void Dragon(SceneObjectList *objectList);
void Micromaterials(SceneObjectList *objectList);

void Micromaterials(GpuTriangleList *objectList);
void whittedScene(GpuTriangleList *objectList);
void NanoSuit(GpuTriangleList *objectList);
void CornellBox(GpuTriangleList *objectList);
void CBox(GpuTriangleList *objectList);
void CBoxWater(GpuTriangleList *objectList);
void teapotScene(GpuTriangleList *objectList);
void Dragon(GpuTriangleList *objectList);
void Sponza(GpuTriangleList *objectList);
