#pragma once

#include "Materials/MaterialManager.h"

#include "Primitives/SceneObjectList.h"
#include "Primitives/TriangleList.h"
//
// void texturedScene(prims::SceneObjectList *objectList);
// void whittedScene(prims::SceneObjectList *objectList);
// void testScene(prims::SceneObjectList *objectList);
// void torusScene(prims::SceneObjectList *objectList);
// void teapotScene(prims::SceneObjectList *objectList);
// void NanoSuit(prims::SceneObjectList *objectList);
//
//// https://github.com/tiansijie/Tile_Based_WebGL_DeferredShader/tree/master/NPRChinesepainting/obj/cornell-box
void CornellBox(prims::SceneObjectList *objectList);
void CornellBox(TriangleList *objectList);
// void CBox(prims::SceneObjectList *objectList);
// void CBoxWater(prims::SceneObjectList *objectList);
// void Sponza(prims::SceneObjectList *objectList);
// void Dragon(prims::SceneObjectList *objectList);

void Micromaterials(prims::SceneObjectList *objectList);
void Micromaterials(TriangleList *objectList);
