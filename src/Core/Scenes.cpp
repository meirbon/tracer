#include "Scenes.h"

#include <glm/ext.hpp>

#include "Primitives/Model.h"
#include "Primitives/Plane.h"
#include "Primitives/Sphere.h"
#include "Primitives/Torus.h"
#include "Primitives/Triangle.h"

void texturedScene(prims::SceneObjectList *objectList)
{
	auto *mManager = MaterialManager::GetInstance();
	const unsigned int tex = mManager->AddTexture("models/cube/wall.jpg");
	const unsigned int tMat = mManager->AddMaterial(Material(1.f, vec3(1.f), 50.f, 1.f, 0.f, vec3(0.f), tex));
	const unsigned int tMat2 = mManager->AddMaterial(Material(.99f, vec3(1.f), 50.f, 1.f, 0.f, vec3(0.f), tex));
	const unsigned int lightMat = mManager->AddMaterial(Material(vec3(15.f), 4.f));

	prims::Load("models/cube/cube.obj", tMat, vec3(0.f, 0, -3.f), .5f, objectList);
	objectList->AddObject(
		new prims::Plane(vec3(0.f, 1.f, 0.f), 1.f, tMat, vec3(-50.f, -1.f, -50.f), vec3(50.f, 1.f, 50.f)));
	objectList->AddObject(new prims::Sphere(vec3(1.f, 0, -3.f), .4f, tMat2));
	objectList->AddObject(
		new prims::Torus(vec3(-1.f, 0, -3.f), vec3(0.f, 0.f, 1.f), vec3(1.f, 0.f, 0.f), .2f, .4f, tMat));
	objectList->AddLight(
		new prims::LightSpot(vec3(0.f, 6.f, -2.f), vec3(0.f, -1.f, 0.f), radians(10.f), radians(45.f), lightMat, 1.f));
}

void whittedScene(prims::SceneObjectList *objectList)
{
	auto *mManager = MaterialManager::GetInstance();
	const vec3 yellow = vec3(float(0xFC) / 255.99f, float(0xEE) / 255.99f, float(0x0E) / 255.59f);
	const vec3 red = vec3(float(0xE0) / 255.99f, float(0x25) / 255.99f, float(0x0B) / 255.59f);
	const unsigned int sphereGlassMaterialIdx =
		mManager->AddMaterial(Material(0.f, vec3(1.f, 1.f, 1.f), 12, 1.54f, .99f, vec3(0.0f)));
	const unsigned int mirrorMaterial = mManager->AddMaterial(Material(.8f, vec3(1.f, .3f, .3f), 4, 1.f, 0, vec3(0.f)));
	const unsigned int whittedPlaneYellow = mManager->AddMaterial(Material(1, yellow, 4, 1.f, 0.f, vec3(0.f)));
	const unsigned int whittedPlaneRed = mManager->AddMaterial(Material(0.1f, red, 4, 1.f, 0.f, vec3(0.f)));
	const unsigned int lightMat = mManager->AddMaterial(Material(vec3(200.f), .5f));

	//	 objectList->AddObject(new Sphere(vec3(0.f, -1.1f, -4.5f), 1.f,
	// sphereGlassMaterialIdx)); 	 objectList->AddObject(new
	// Sphere(vec3(1.8f,
	//-.5f, -6.5f), 1.f, mirrorMaterial)); 	 objectList->AddObject(new
	// Sphere(vec3(0.f, 8.f, 0.f), 1.f, lightMat)); objectList->AddObject(new
	// Plane(vec3(0, 1, 0), 0, whittedPlaneRed, vec3(-50, -EPSILON, -50),
	// vec3(50, EPSILON, 50)));

	prims::Load("models/sphere.obj", sphereGlassMaterialIdx, vec3(0.f, -1.1f, -4.5f), .05f, objectList);
	prims::Load("models/sphere.obj", mirrorMaterial, vec3(1.8f, -.5f, -6.5f), .05f, objectList);
	prims::Load("models/sphere.obj", lightMat, vec3(0.f, 8.f, 0.f), .05f, objectList);
	prims::Load("models/cube/cube.obj", sphereGlassMaterialIdx, vec3(0.f, 0, -3.f), .5f, objectList);

	int zIdx = 0;
	for (int z = -39; z < 1; z++)
	{
		int idx = zIdx;
		for (int x = -2; x < 10; x++)
		{
			auto tp = prims::TrianglePlane(vec3(float(x), -2.5f, float(z)),
										   vec3(float(x - 1) + 2.0f * EPSILON, -2.5f, float(z)),
										   vec3(float(x), -2.5f, float(z - 1) + 2.0f * EPSILON),
										   idx ? whittedPlaneRed : whittedPlaneYellow, objectList);
			idx = (idx + 1) % 2;
		}
		zIdx = (zIdx + 1) % 2;
	}
}

void testScene(prims::SceneObjectList *objectList)
{
	auto *mManager = MaterialManager::GetInstance();

	const unsigned int sphereGlassMaterialIdx =
		mManager->AddMaterial(Material(0.01f, vec3(0.5f, 0.5f, 0.7f), 4, 1.54f, 1.f, vec3(0.f)));
	const unsigned int teapotMaterial =
		mManager->AddMaterial(Material(1.f, vec3(.6f, .3f, .3f), 2, 1.f, 0.f, vec3(0.f)));
	const unsigned int planeMaterialIdx =
		mManager->AddMaterial(Material(.3f, vec3(0.3f, 0.2f, 0.3f), 2, 1.f, 0.f, vec3(0.f)));
	const unsigned int sphereMaterialIdx =
		mManager->AddMaterial(Material(.2f, vec3(0.7f, 0.5f, 0.5f), 50.f, 1.f, 0.f, vec3(0.f, 0.f, 0.f)));
	const unsigned int redSphereMaterialIdx =
		mManager->AddMaterial(Material(0.5f, vec3(0.3f, 0.3f, 0.7f), 50.f, 1.f, 0.99f, vec3(1.f, 1.f, 0.f)));
	const unsigned int objectMaterialIdx =
		mManager->AddMaterial(Material(1.f, vec3(0.4f, 0.5f, 0.4f), 50.f, 1.f, 0.f, vec3(0.f, 0.f, 0.f)));
	const unsigned int lightMat2 = mManager->AddMaterial(Material(vec3(100.f, 100.f, 70.f), 1.f));
	const unsigned int lightMat3 = mManager->AddMaterial(Material(vec3(80.f, 80.f, 80.f), 1.f));

	prims::Load("models/cube/cube.obj", objectMaterialIdx, vec3(0.f, .2f, -4.f), .5f, objectList);
	prims::Load("models/teapot.obj", teapotMaterial, vec3(-3.f, -1.f, -7.f), .7f, objectList);

	objectList->AddObject(new prims::Sphere(vec3(1.f, 0.f, -3.f), .2f, redSphereMaterialIdx));
	objectList->AddObject(new prims::Sphere(vec3(2.f, -.3f, -5.f), 0.4f, sphereMaterialIdx));
	objectList->AddObject(new prims::Sphere(vec3(-1.f, -.3f, -4.f), 0.4f, sphereGlassMaterialIdx));
	objectList->AddObject(
		new prims::Plane(vec3(0.f, 1.f, 0.f), 1.f, planeMaterialIdx, vec3(-50.f, -1.f, -50.f), vec3(50.f, 1.f, 50.f)));
	objectList->AddLight(new prims::LightPoint(vec3(4.f, 6.5f, 0.5f), 1.f, lightMat2));
	objectList->AddLight(new prims::LightPoint(vec3(0.f, 7, -5.f), 1.f, lightMat3));
}

void torusScene(prims::SceneObjectList *objectList)
{
	auto *mManager = MaterialManager::GetInstance();

	const unsigned int objectMaterialIdx =
		mManager->AddMaterial(Material(0.8f, vec3(0.2f, 0.2f, 0.9f), 32.f, 1.f, 0.f, vec3(0.f, 0.f, 0.f)));
	const unsigned int planeMaterialIdx =
		mManager->AddMaterial(Material(.8f, vec3(0.3f), 2.f, 1.f, 0.f, vec3(0.f, 0.f, 0.f)));
	const unsigned int tMat = mManager->AddMaterial(
		Material(1.f, vec3(1.f), 50.f, 1.f, 0.f, vec3(0.f), mManager->AddTexture("models/cube/wall.jpg")));
	const unsigned int lightMat = mManager->AddMaterial(Material(vec3(1.f), 50.f));

	objectList->AddObject(
		new prims::Torus(vec3(-1.f, .2f, -4.f), vec3(0.f, 1.f, 0.f), vec3(0.f, 0.f, 1.f), .2f, .6f, tMat));
	objectList->AddObject(
		new prims::Torus(vec3(1.f, .2f, -4.f), vec3(0.f, 1.f, 0.f), vec3(0.f, 0.f, 1.f), .2f, .6f, objectMaterialIdx));
	objectList->AddObject(new prims::Plane(vec3(0.f, 1.f, 0.f), 2.f, planeMaterialIdx, vec3(-10.f, -EPSILON, -10.f),
										   vec3(10.f, EPSILON, 10.f)));
	objectList->AddLight(new prims::LightPoint(vec3(4.f, 6.5f, 0.5f), 2.f, lightMat));
}

void teapotScene(prims::SceneObjectList *objectList)
{
	auto *mManager = MaterialManager::GetInstance();

	const unsigned int mirrorMaterial =
		mManager->AddMaterial(Material(1.0f, vec3(1.f, 1.f, 1.f), 4, 1.f, 0.f, vec3(0.f)));
	for (int x = -45; x < 46; x += 5)
	{
		for (int z = -45; z < 46; z += 5)
		{
			prims::Load("models/teapot.obj", mirrorMaterial, vec3(x, -1.f, z), .7f, objectList);
		}
	}

	//	prims::Load("models/teapot.obj", mirrorMaterial, vec3(0.f, -1.f,
	// 5.f), 1.5f, objectList, objectList);
	const unsigned int planeMaterialIdx =
		mManager->AddMaterial(Material(.1f, vec3(1.f, .7f, .7f), 2, 1.f, 0.f, vec3(0.f)));
	prims::TrianglePlane(vec3(50.f, 0.f, 50.f), vec3(-50.f, -1.f, 50.f), vec3(50.f, -1.f, -50.f), planeMaterialIdx,
						 objectList);
}

void NanoSuit(prims::SceneObjectList *objectList)
{
	auto *mManager = MaterialManager::GetInstance();

	// const unsigned int lightMat1 =
	// objectList->AddMaterial(Material(vec3(100.f, 120.f,
	// 80.f), 1.f));
	const unsigned int teapotMaterial =
		mManager->AddMaterial(Material(0.f, vec3(.3f, .3f, .6f), 2, 1.2f, 1.0f, vec3(0.f)));
	const unsigned int planeMaterialIdx = mManager->AddMaterial(Material(.3f, vec3(1.f), 2, 1.f, 0.f, vec3(0.f)));

	// prims::Load("models/sphere.obj", lightMat1, vec3(0.f, 12.f, 10.f), .1f,
	// objectList, objectList);
	auto tp = prims::TrianglePlane(vec3(50.f, -1.f, 50.f), vec3(-50.f, -1.f, 50.f), vec3(50.f, -1.f, -50.f),
								   planeMaterialIdx, objectList);
	prims::Load("models/nanosuit/nanosuit.obj", teapotMaterial, vec3(0.f, -1.f, -5.f), 1.f, objectList);
}

void CornellBox(prims::SceneObjectList *objectList)
{
	auto *mManager = MaterialManager::GetInstance();

	const unsigned int objectMaterialIdx =
		mManager->AddMaterial(Material(1.f, vec3(0.4f, 0.5f, 0.4f), 50.f, 1.f, 0.f, vec3(0.f, 0.f, 0.f)));
	prims::Load("models/cornellbox/CornellBox-Original.obj", objectMaterialIdx, vec3(0.f, -2.5f, -11.f), 5.f,
				objectList);
}

void CBox(prims::SceneObjectList *objectList)
{
	auto *mManager = MaterialManager::GetInstance();

	const unsigned int objectMaterialIdx =
		mManager->AddMaterial(Material(1.f, vec3(0.4f, 0.5f, 0.4f), 50.f, 1.f, 0.f, vec3(0.f, 0.f, 0.f)));
	prims::Load("models/cbox/CornellBox-Sphere.obj", objectMaterialIdx, vec3(0.f, -2.5f, -11.f), 5.f, objectList);
}

void CBoxWater(prims::SceneObjectList *objectList)
{
	auto *mManager = MaterialManager::GetInstance();

	const unsigned int objectMaterialIdx =
		mManager->AddMaterial(Material(1.f, vec3(0.4f, 0.5f, 0.4f), 50.f, 1.f, 0.f, vec3(0.f, 0.f, 0.f)));
	prims::Load("models/cbox/CornellBox-Water.obj", objectMaterialIdx, vec3(0.f, -2.5f, -11.f), 5.f, objectList);
}

void Sponza(prims::SceneObjectList *objectList)
{
	auto *mManager = MaterialManager::GetInstance();

	const unsigned int lightMat = mManager->AddMaterial(Material(vec3(100.0f), .5f));
	const unsigned int sponza = mManager->AddMaterial(Material(1, vec3(1.f, .95f, .8f), 4, 1.f, 0.f, vec3(0.f)));
	prims::Load("models/sphere.obj", lightMat, vec3(0.f, 30.f, 0.f), .2f, objectList);
	prims::Load("models/sponza/sponza.obj", sponza, vec3(0.f, 0.f, 0.f), .2f, objectList);
}

void Dragon(prims::SceneObjectList *objectList)
{
	auto *mManager = MaterialManager::GetInstance();

	const unsigned int lightMat = mManager->AddMaterial(Material(vec3(100.f), .5f));
	prims::Load("models/sphere.obj", lightMat, vec3(0.f, 8.f, 0.f), .05f, objectList);

	const unsigned int dragMat =
		mManager->AddMaterial(Material(0.0001f, vec3(.8f, .8f, 1.f), 4, 1.2f, 1.f, vec3(0.0f, 0.3f, 0.3f)));
	prims::Load("models/dragon.obj", dragMat, vec3(0.f, -.98f, -15.f), 5.0f, objectList,
				glm::rotate(glm::mat4(1.0f), glm::radians(-90.f), vec3(1, 0, 0)));

	const unsigned int planeMaterialIdx =
		mManager->AddMaterial(Material(0.01f, vec3(1.f, .7f, .7f), 2, 1.f, 0.f, vec3(0.f)));
	prims::TrianglePlane(vec3(50.f, 0.f, 50.f), vec3(-50.f, -1.f, 50.f), vec3(50.f, -1.f, -50.f), planeMaterialIdx,
						 objectList);
}

void Micromaterials(prims::SceneObjectList *objectList)
{
	auto *mManager = MaterialManager::GetInstance();

	const vec3 yellow = vec3(float(0xFC) / 255.99f, float(0xEE) / 255.99f, float(0x0E) / 255.59f);
	const vec3 red = vec3(float(0xE0) / 255.99f, float(0x25) / 255.99f, float(0x0B) / 255.59f);
	const unsigned int sphereMaterialIdx1 =
		mManager->AddMaterial(Material(1.f, vec3(1.f, 1.f, 1.f), 12, 1.54f, 0.f, vec3(0.0f)));
	const unsigned int sphereMaterialIdx2 =
		mManager->AddMaterial(Material(0.5f, vec3(1.f, 1.f, 1.f), 12, 1.54f, 0.f, vec3(0.0f)));
	const unsigned int sphereMaterialIdx3 =
		mManager->AddMaterial(Material(0.2f, vec3(1.f, 1.f, 1.f), 12, 1.54f, 0.f, vec3(0.0f)));
	const unsigned int sphereMaterialIdx4 =
		mManager->AddMaterial(Material(0.05f, vec3(1.f, 1.f, 1.f), 12, 1.54f, 0.f, vec3(0.0f)));
	const unsigned int sphereMaterialIdx5 =
		mManager->AddMaterial(Material(0.0001f, vec3(1.f, 1.f, 1.f), 12, 1.54f, 0.f, vec3(0.0f)));
	const unsigned int whittedPlaneYellow = mManager->AddMaterial(Material(1, yellow, 4, 1.f, 0.f, vec3(0.f)));
	const unsigned int whittedPlaneRed = mManager->AddMaterial(Material(0.01f, red, 4, 1.f, 0.f, vec3(0.f)));
	const unsigned int lightMat = mManager->AddMaterial(Material(vec3(5.0f), .5f));

	prims::Load("models/sphere.obj", sphereMaterialIdx1, vec3(-5.f, -1.1f, -4.5f), .05f, objectList);
	prims::Load("models/sphere.obj", sphereMaterialIdx2, vec3(-2.5f, -1.1f, -4.5f), .05f, objectList);
	prims::Load("models/sphere.obj", sphereMaterialIdx3, vec3(0.f, -1.1f, -4.5f), .05f, objectList);
	prims::Load("models/sphere.obj", sphereMaterialIdx4, vec3(2.5f, -1.1f, -4.5f), .05f, objectList);
	prims::Load("models/sphere.obj", sphereMaterialIdx5, vec3(5.f, -1.1f, -4.5f), .05f, objectList);

	auto tp = prims::TrianglePlane(vec3(4.0f, 6.0f, 1.0f), vec3(-4.0f, 6.0f, 1.0f), vec3(4.0f, 6.0f, -3.0f), lightMat,
								   objectList);

	int zIdx = 0;
	for (int z = -10; z < 10; z++)
	{
		int idx = zIdx;
		for (int x = -20; x < 2; x++)
		{
			auto t = prims::TrianglePlane(
				vec3(float(z), -2.5f, float(x)), vec3(float(z - 1) + EPSILON, -2.5f, float(x)),
				vec3(float(z), -2.5f, float(x - 1) + EPSILON), idx ? whittedPlaneRed : whittedPlaneYellow, objectList);
			idx = (idx + 1) % 2;
		}
		zIdx = (zIdx + 1) % 2;
	}
}

void Micromaterials(prims::GpuTriangleList *objectList)
{
	auto *mManager = MaterialManager::GetInstance();

	const vec3 yellow = vec3(float(0xFC) / 255.99f, float(0xEE) / 255.99f, float(0x0E) / 255.59f);
	const vec3 red = vec3(float(0xE0) / 255.99f, float(0x25) / 255.99f, float(0x0B) / 255.59f);
	const unsigned int sphereMaterialIdx1 =
		mManager->AddMaterial(Material(1.f, vec3(1.f, 1.f, 1.f), 12, 1.54f, 0.f, vec3(0.0f)));
	const unsigned int sphereMaterialIdx2 =
		mManager->AddMaterial(Material(0.5f, vec3(1.f, 1.f, 1.f), 12, 1.54f, 0.f, vec3(0.0f)));
	const unsigned int sphereMaterialIdx3 =
		mManager->AddMaterial(Material(0.2f, vec3(1.f, 1.f, 1.f), 12, 1.54f, 0.f, vec3(0.0f)));
	const unsigned int sphereMaterialIdx4 =
		mManager->AddMaterial(Material(0.1f, vec3(1.f, 1.f, 1.f), 12, 1.54f, 0.f, vec3(0.0f)));
	const unsigned int sphereMaterialIdx5 =
		mManager->AddMaterial(Material(0.01f, vec3(1.f, 1.f, 1.f), 12, 1.54f, 0.f, vec3(0.0f)));
	const unsigned int whittedPlaneYellow = mManager->AddMaterial(Material(1, yellow, 4, 1.f, 0.f, vec3(0.f)));
	const unsigned int whittedPlaneRed = mManager->AddMaterial(Material(0.01f, red, 4, 1.f, 0.f, vec3(0.f)));
	const unsigned int lightMat = mManager->AddMaterial(Material(vec3(1.f), .5f));

	prims::Load("models/sphere.obj", sphereMaterialIdx1, vec3(-5.f, -1.1f, -4.5f), .05f, objectList);
	prims::Load("models/sphere.obj", sphereMaterialIdx2, vec3(-2.5f, -1.1f, -4.5f), .05f, objectList);
	prims::Load("models/sphere.obj", sphereMaterialIdx3, vec3(0.f, -1.1f, -4.5f), .05f, objectList);
	prims::Load("models/sphere.obj", sphereMaterialIdx4, vec3(2.5f, -1.1f, -4.5f), .05f, objectList);
	prims::Load("models/sphere.obj", sphereMaterialIdx5, vec3(5.f, -1.1f, -4.5f), .05f, objectList);

	auto tp = prims::TrianglePlane(vec3(4.0f, 6.0f, 1.0f), vec3(-4.0f, 6.0f, 1.0f), vec3(4.0f, 6.0f, -3.0f), lightMat,
								   objectList);
	// prims::Load("models/sphere.obj", lightMat, vec3(0.f, 8.f, 0.f), .05f,
	// objectList);

	int zIdx = 0;
	for (int z = -10; z < 10; z++)
	{
		int idx = zIdx;
		for (int x = -20; x < 2; x++)
		{
			auto t = prims::TrianglePlane(
				vec3(float(z), -2.5f, float(x)), vec3(float(z - 1) + EPSILON, -2.5f, float(x)),
				vec3(float(z), -2.5f, float(x - 1) + EPSILON), idx ? whittedPlaneRed : whittedPlaneYellow, objectList);
			idx = (idx + 1) % 2;
		}
		zIdx = (zIdx + 1) % 2;
	}
}

void whittedScene(prims::GpuTriangleList *objectList)
{
	auto *mManager = MaterialManager::GetInstance();

	const vec3 yellow = vec3(float(0xFC) / 255.99f, float(0xEE) / 255.99f, float(0x0E) / 255.59f);
	const vec3 red = vec3(float(0xE0) / 255.99f, float(0x25) / 255.99f, float(0x0B) / 255.59f);
	const unsigned int sphereGlassMaterialIdx =
		mManager->AddMaterial(Material(0.f, vec3(1.f, 1.f, 1.f), 12, 1.54f, .99f, vec3(0.f)));
	const unsigned int mirrorMaterial = mManager->AddMaterial(Material(.8f, vec3(1.f, .3f, .3f), 4, 1.f, 0, vec3(0.f)));
	const unsigned int whittedPlaneYellow = mManager->AddMaterial(Material(1.0f, yellow, 4, 1.f, 0.f, vec3(0.f)));
	const unsigned int whittedPlaneRed = mManager->AddMaterial(Material(0.1f, red, 4, 1.f, 0.f, vec3(0.f)));
	const unsigned int lightMat = mManager->AddMaterial(Material(vec3(200.f), .5f));

	prims::Load("models/sphere.obj", sphereGlassMaterialIdx, vec3(0.f, -1.1f, -4.5f), .05f, objectList);
	prims::Load("models/sphere.obj", mirrorMaterial, vec3(1.8f, -.5f, -6.5f), .05f, objectList);
	prims::Load("models/sphere.obj", lightMat, vec3(0.f, 8.f, 0.f), .05f, objectList);
	prims::Load("models/cube/cube.obj", sphereGlassMaterialIdx, vec3(0.f, 0, -3.f), .5f, objectList);
	prims::Load("models/cube/cube2.obj", sphereGlassMaterialIdx, vec3(1.f, .5f, -3.f), .5f, objectList);

	int zIdx = 0;
	for (int z = -39; z < 1; z++)
	{
		int idx = zIdx;
		for (int x = -2; x < 10; x++)
		{
			auto tp = prims::TrianglePlane(vec3(float(x), -2.5f, float(z)),
										   vec3(float(x - 1) + 2.0f * EPSILON, -2.5f, float(z)),
										   vec3(float(x), -2.5f, float(z - 1) + 2.0f * EPSILON),
										   idx ? whittedPlaneRed : whittedPlaneYellow, objectList);
			idx = (idx + 1) % 2;
		}
		zIdx = (zIdx + 1) % 2;
	}
}

void NanoSuit(prims::GpuTriangleList *objectList)
{
	auto *mManager = MaterialManager::GetInstance();

	const unsigned int lightMat1 = mManager->AddMaterial(Material(vec3(10.f, 12.f, 8.f), 1.f));
	const unsigned int planeMaterialIdx = mManager->AddMaterial(Material(.01f, vec3(1.f), 2, 1.f, 0.f, vec3(0.f)));

	prims::Load("models/sphere.obj", lightMat1, vec3(0.f, 12.f, 10.f), .1f, objectList);
	auto tp = prims::TrianglePlane(vec3(50.f, 0.f, 50.f), vec3(-50.f, -1.f, 50.f), vec3(50.f, -1.f, -50.f),
								   planeMaterialIdx, objectList);
	prims::Load("models/nanosuit/nanosuit.obj", 0, vec3(0.f, -1.f, -5.f), 1.f, objectList);
}

void CornellBox(prims::GpuTriangleList *objectList)
{
	auto *mManager = MaterialManager::GetInstance();

	const unsigned int objectMaterialIdx =
		mManager->AddMaterial(Material(1.f, vec3(0.4f, 0.5f, 0.4f), 50.f, 1.f, 0.f, vec3(0.f, 0.f, 0.f)));
	prims::Load("models/cornellbox/CornellBox-Original.obj", objectMaterialIdx, vec3(0.f, -2.5f, -11.f), 5.f,
				objectList);
}

void CBox(prims::GpuTriangleList *objectList)
{
	auto *mManager = MaterialManager::GetInstance();

	const unsigned int objectMaterialIdx =
		mManager->AddMaterial(Material(1.f, vec3(0.4f, 0.5f, 0.4f), 50.f, 1.f, 0.f, vec3(0.f, 0.f, 0.f)));
	prims::Load("models/cbox/CornellBox-Sphere.obj", objectMaterialIdx, vec3(0.f, -2.5f, -11.f), 5.f, objectList);
}

void CBoxWater(prims::GpuTriangleList *objectList)
{
	auto *mManager = MaterialManager::GetInstance();

	const unsigned int objectMaterialIdx =
		mManager->AddMaterial(Material(1.f, vec3(0.4f, 0.5f, 0.4f), 50.f, 1.f, 0.f, vec3(0.f, 0.f, 0.f)));
	prims::Load("models/cbox/CornellBox-Water.obj", objectMaterialIdx, vec3(0.f, -2.5f, -11.f), 5.f, objectList);
}

void teapotScene(prims::GpuTriangleList *objectList)
{
	auto *mManager = MaterialManager::GetInstance();

	const unsigned int mirrorMaterial =
		mManager->AddMaterial(Material(1.0f, vec3(1.f, 1.f, 1.f), 4, 1.f, 0.f, vec3(0.f)));
	//    for (int x = -45; x < 46; x += 5) {
	//        for (int z = -45; z < 46; z += 5) {
	//            prims::Load("models/teapot.obj", mirrorMaterial, vec3(x, -1.f,
	//            z), .7f, objectList);
	//        }
	//    }

	//	const unsigned int mirrorMaterial =
	// mManager->AddMaterial(Material(.1f, vec3(1.f, 1.f, 1.f),
	// 4, 1.f, 0.f, vec3(0.f)));
	prims::Load("models/teapot.obj", mirrorMaterial, vec3(0.f, -1.f, -5.f), 1.5f, objectList);

	const unsigned int planeMaterialIdx =
		mManager->AddMaterial(Material(0.2f, vec3(.3f, .1f, .1f), 2, 1.f, 0.f, vec3(0.f)));
	prims::TrianglePlane(vec3(50.f, 0.f, 50.f), vec3(-50.f, -1.f, 50.f), vec3(50.f, -1.f, -50.f), planeMaterialIdx,
						 objectList);
}

void Dragon(prims::GpuTriangleList *objectList)
{
	auto *mManager = MaterialManager::GetInstance();

	const unsigned int lightMat = mManager->AddMaterial(Material(vec3(100.f), .5f));
	prims::Load("models/sphere.obj", lightMat, vec3(0.f, 17.f, -10.f), .05f, objectList);

	const unsigned int dragMat =
		mManager->AddMaterial(Material(0.0001f, vec3(.8f, .8f, 1.f), 4, 1.2f, 1.f, vec3(0.0f, 0.3f, 0.3f)));
	prims::Load("models/dragon.obj", dragMat, vec3(0.f, -.98f, -15.f), 5.0f, objectList,
				glm::rotate(glm::mat4(1.0f), glm::radians(-90.f), vec3(1, 0, 0)));

	const unsigned int planeMaterialIdx = mManager->AddMaterial(Material(1.0f, vec3(1.f), 2, 1.f, 0.f, vec3(0.f)));
	prims::TrianglePlane(vec3(50.f, 0.f, 50.f), vec3(-50.f, -1.f, 50.f), vec3(50.f, -1.f, -50.f), planeMaterialIdx,
						 objectList);
}

void Sponza(prims::GpuTriangleList *objectList)
{
	auto *mManager = MaterialManager::GetInstance();

	const unsigned int sponza = mManager->AddMaterial(Material(1, vec3(1.f, .95f, .8f), 4, 1.f, 0.f, vec3(0.f)));
	prims::Load("models/sponza/sponza.obj", sponza, vec3(0.f, 0.f, 0.f), .2f, objectList);
}

void Micromaterials(TriangleList *objectList)
{
	using namespace prims;
	auto *mManager = MaterialManager::GetInstance();

	const vec3 yellow = vec3(float(0xFC) / 255.99f, float(0xEE) / 255.99f, float(0x0E) / 255.59f);
	const vec3 red = vec3(float(0xE0) / 255.99f, float(0x25) / 255.99f, float(0x0B) / 255.59f);

	const auto lightMat = Material::light(vec3(1.0f));
	const auto yellowMat = Material::lambertian(yellow);
	const auto redMat = Material::specular(red, 1.0f);
	const auto sphereMat = Material::lambertian(vec3(1.0f, 0.7f, 0.7f));

	const unsigned int sphereIdx = mManager->AddMaterial(sphereMat);
	const unsigned int yellowIdx = mManager->AddMaterial(yellowMat);
	const unsigned int redIdx = mManager->AddMaterial(redMat);
	const unsigned int lightIdx = mManager->AddMaterial(lightMat);

	objectList->loadModel("models/sphere.obj", 1.0f, translate(mat4(1.0f), vec3(-5.f, -1.1f, -4.5f)), sphereIdx);
	objectList->loadModel("models/sphere.obj", 1.0f, translate(mat4(1.0f), vec3(-2.5f, -1.1f, -4.5f)), sphereIdx);
	objectList->loadModel("models/sphere.obj", 1.0f, translate(mat4(1.0f), vec3(0.0f, -1.1f, -4.5f)), sphereIdx);
	objectList->loadModel("models/sphere.obj", 1.0f, translate(mat4(1.0f), vec3(2.5f, -1.1f, -4.5f)), sphereIdx);
	objectList->loadModel("models/sphere.obj", 1.0f, translate(mat4(1.0f), vec3(5.0f, -1.1f, -4.5f)), sphereIdx);

	TrianglePlane::create(vec3(4.0f, 6.0f, 1.0f), vec3(-4.0f, 6.0f, 1.0f), vec3(4.0f, 6.0f, -3.0f), lightIdx,
						  objectList);

	int zIdx = 0;
	for (int z = -10; z < 10; z++)
	{
		int idx = zIdx;
		for (int x = -20; x < 2; x++)
		{
			prims::TrianglePlane::create(vec3(float(z), -2.5f, float(x)), vec3(float(z - 1) + EPSILON, -2.5f, float(x)),
										 vec3(float(z), -2.5f, float(x - 1) + EPSILON), idx ? redIdx : yellowIdx,
										 objectList);
			idx = (idx + 1) % 2;
		}
		zIdx = (zIdx + 1) % 2;
	}
}
