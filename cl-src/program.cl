#include "include.cl"

typedef struct
{
	float3 origin;
	float3 viewDirection;
	float3 vertical;
	float3 horizontal;

	float viewDistance;
	float invWidth;
	float invHeight;
	float aspectRatio;
} GpuCamera;

kernel void generate(write_only image2d_t outimg, // 0
					 global float4 *colorBuffer,  // 1
					 global Ray *rays,			  // 2
					 global ShadowRay *sRays,	 // 3
					 global GpuCamera *camera,	// 4
					 int width,					  // 5
					 int height,				  // 6
					 int frame					  // 7
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

		r.direction = normalize(camera->viewDirection + camera->horizontal * ScreenX + camera->vertical * ScreenY);
		r.hit_idx = RAY_MASK_NO_HIT;

		r.throughput = (float3)(1, 1, 1);
		r.lastNormal = (float3)(0);
		r.bounces = 0;
		r.lastMatType = 0;

		rays[pixelIdx] = r;
		sRays[pixelIdx].hit_idx = -2;
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

	iMBVHTree(nodes, indices, vertices, primitiveIndices, &ray.t, &ray.hit_idx, ray.origin, ray.direction);
	rays[pixelIdx] = ray;
}

kernel void connect(global ShadowRay *sRays,	   // 0
					global int3 *indices,		   // 1
					global float3 *vertices,	   // 2
					global uint *primitiveIndices, // 3
					global float4 *colorBuffer,	// 4
					global MBVHNode *nodes,		   // 5
					int width,					   // 6
					int height					   // 7
)
{
	int intx = get_global_id(0);
	int inty = get_global_id(1);
	if (intx >= width || inty >= height)
		return;
	int pixelIdx = intx + inty * width;

	ShadowRay ray = sRays[pixelIdx];
	if (ray.hit_idx != -2)
	{
		iMBVHTree(nodes, indices, vertices, primitiveIndices, &ray.t, &ray.hit_idx, ray.origin, ray.direction);
		if (ray.hit_idx == -1)
			colorBuffer[pixelIdx] += (float4)(ray.color, 0.0f);
	}
}

kernel void draw(write_only image2d_t outimg, // 0
				 global float4 *colorBuffer,  // 1
				 int width,					  // 2
				 int height,				  // 3
				 int frames					  // 4
)
{
	int intx = get_global_id(0);
	int inty = get_global_id(1);
	if (intx >= width || inty >= height)
		return;
	int pixelIdx = intx + inty * width;

	const float4 color = colorBuffer[pixelIdx];
	const float3 exponent = (float3)(1.0f / 2.2f);
	const float3 col = pow(color.xyz / (fmax(color.w, 1.0f)), exponent);

	write_imagef(outimg, (int2)(intx, inty), (float4)(col, 1.f));
}