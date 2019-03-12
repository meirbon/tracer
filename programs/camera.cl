#ifndef CAMERA_H
#define CAMERA_H

typedef struct GpuCamera {
	float3 origin;
	float3 viewDirection;

	float viewDistance;
	float invWidth;
	float invHeight;
	float aspectRatio;
} GpuCamera;

#endif