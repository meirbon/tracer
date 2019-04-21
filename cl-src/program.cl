#include "../src/Shared.h"
// clang-format off
#include "random.cl"
#include "material.cl"
#include "ray.cl"
#include "triangle.cl"
#include "mbvh.cl"
#include "microfacet.cl"
// clang-format on

typedef struct
{
	float3 origin;
	float3 viewDirection;

	float viewDistance;
	float invWidth;
	float invHeight;
	float aspectRatio;
} GpuCamera;

kernel void generate(write_only image2d_t outimg, // 0
					 global float4 *colorBuffer,  // 1
					 global Ray *rays,			  // 2
					 global GpuCamera *camera,	// 3
					 float3 horizontal,			  // 4
					 float3 vertical,			  // 5
					 int width,					  // 6
					 int height,				  // 7
					 int frame					  // 8
)
{
	int intx = get_global_id(0);
	int inty = get_global_id(1);
	if (intx >= width || inty >= height)
		return;
	int pixelIdx = intx + inty * width;

	if (frame <= 1)
	{
		colorBuffer[pixelIdx] = (float4)(0);
		write_imagef(outimg, (int2)(intx, inty), (float4)(0, 0, 0, 1));
	}

	// Only regenerate rays that are invalid, others will keep going
	if (rays[pixelIdx].hit_idx < 0 || frame <= 1)
	{
		uint seed = (pixelIdx + frame * 147565741) * 720898027;

		const float x = (float)intx + RandomFloat(&seed) - .5f;
		const float y = (float)inty + RandomFloat(&seed) - .5f;

		const float PixelX = x * camera->invWidth;
		const float PixelY = y * camera->invHeight;
		const float ScreenX = 2.f * PixelX - 1.f;
		const float ScreenY = 1.f - 2.f * PixelY;

		Ray r;

		r.origin = camera->origin;
		r.t = 1e34f;

		r.direction = normalize(camera->viewDirection + horizontal * ScreenX + vertical * ScreenY);
		r.hit_idx = -1;

		r.throughput = (float3)(1, 1, 1);
		r.lastNormal = (float3)(0);
		r.bounces = 0;

		rays[pixelIdx] = r;
	}
}

kernel void intersect(global Ray *rays,				 // 0
					  global int3 *indices,			 // 1
					  global float3 *vertices,		 // 2
					  global MBVHNode *nodes,		 // 3
					  global uint *primitiveIndices, // 4
					  int width,					 // 5
					  int height					 // 6
)
{
	int intx = get_global_id(0);
	int inty = get_global_id(1);
	if (intx >= width || inty >= height)
		return;
	int pixelIdx = intx + inty * width;

	Ray ray = rays[pixelIdx];
	ray.hit_idx = -1;
	ray.t = 1e34f;

	iMBVHTree(nodes, indices, vertices, primitiveIndices, &ray);
	rays[pixelIdx] = ray;
}

kernel void shade(global Ray *rays,				   // 0
				  global Material *materials,	  // 1
				  global int3 *indices,			   // 2
				  global float3 *vertices,		   // 3
				  global float3 *cns,			   // 4
				  global float3 *normals,		   // 5
				  global float2 *texCoords,		   // 6
				  global uint *primitiveIndices,   // 7
				  global float4 *colorBuffer,	  // 8
				  global float3 *textureBuffer,	// 9
				  global TextureInfo *textureInfo, // 10
				  global float3 *skyDome,		   // 11
				  global TextureInfo *skyInfo,	 // 12
				  global Microfacet *microfacets,  // 13
				  int hasSkyDome,				   // 14
				  int width,					   // 15
				  int height,					   // 16
				  int frame						   // 17
)
{
	int intx = get_global_id(0);
	int inty = get_global_id(1);
	if (intx >= width || inty >= height)
		return;
	int pixelIdx = intx + inty * width;

	Ray r = rays[pixelIdx];
	if (!r_valid(r))
		return;

	float3 p = r.origin + r.t * r.direction;
	int3 idx = indices[r.hit_idx];
	float3 cNormal = cns[r.hit_idx];
	float3 bary = t_getBaryCoords(p, cNormal, vertices[idx.x], vertices[idx.y], vertices[idx.z]);
	float3 normal = t_getNormal(bary, normals[idx.x], normals[idx.y], normals[idx.z]);

	colorBuffer[pixelIdx] = (float4)(normal, 1);
}

kernel void draw(write_only image2d_t outimg, // 0
				 global float4 *colorBuffer,  // 1
				 int samples,				  // 2
				 int width,					  // 3
				 int height					  // 4
)
{
	int intx = get_global_id(0);
	int inty = get_global_id(1);
	if (intx >= width || inty >= height)
		return;
	int pixelIdx = intx + inty * width;

	const float4 color = colorBuffer[pixelIdx];

	if (color.w > 0.0f)
	{
		const float exponent = 1.0f / 2.2f;
		const float3 temp = color.xyz / colorBuffer[pixelIdx].w;
		const float3 col = pow(temp, (float3)(exponent));

		write_imagef(outimg, (int2)(intx, inty), (float4)(col, 1.f));
	}
}