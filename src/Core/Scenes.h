#pragma once

#include "Materials/MaterialManager.h"

#include "Primitives/SceneObjectList.h"
#include "Primitives/GpuTriangleList.h"

void texturedScene(prims::SceneObjectList *objectList);
void whittedScene(prims::SceneObjectList *objectList);
void testScene(prims::SceneObjectList *objectList);
void torusScene(prims::SceneObjectList *objectList);
void teapotScene(prims::SceneObjectList *objectList);
void NanoSuit(prims::SceneObjectList *objectList);

// https://github.com/tiansijie/Tile_Based_WebGL_DeferredShader/tree/master/NPRChinesepainting/obj/cornell-box
void CornellBox(prims::SceneObjectList *objectList);
void CBox(prims::SceneObjectList *objectList);
void CBoxWater(prims::SceneObjectList *objectList);
void Sponza(prims::SceneObjectList *objectList);
void Dragon(prims::SceneObjectList *objectList);
void Micromaterials(prims::SceneObjectList *objectList);

void Micromaterials(prims::GpuTriangleList *objectList);
void whittedScene(prims::GpuTriangleList *objectList);
void NanoSuit(prims::GpuTriangleList *objectList);
void CornellBox(prims::GpuTriangleList *objectList);
void CBox(prims::GpuTriangleList *objectList);
void CBoxWater(prims::GpuTriangleList *objectList);
void teapotScene(prims::GpuTriangleList *objectList);
void Dragon(prims::GpuTriangleList *objectList);
void Sponza(prims::GpuTriangleList *objectList);
