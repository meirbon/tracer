#define PI 3.14159265358979323846f
#define INVPI (1.0f / PI)
#define DEBUG_BVH 0
#define DEBUG_RAND 0
#define SAMPLE_COUNT 1
#define MAX_DEPTH 10
#define VARIANCE_REDUCTION 0
#define MICROFACETS 1

#include "include.cl"

kernel void generate_wf(write_only image2d_t outimg, global float4 *colorBuffer, global Ray *rays, global GpuCamera *camera, float3 horizontal,
						float3 vertical, int width, int height, int frame)
{
	unsigned int intx = get_global_id(0);
	unsigned int inty = get_global_id(1);
	const unsigned int pixelIdx = intx + inty * width;
	if (pixelIdx >= (width * height))
		return;

	if (frame <= 1) {
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

kernel void intersect_mf(global Ray *rays,					// 0
						 global Material *materials,		// 1
						 global Triangle *triangles,		// 2
						 global BVHNode *nodes,				// 3
						 global MBVHNode *mNodes,			// 4
						 global uint *primitiveIndices,		// 5
						 global uint *seeds,				// 6
						 global float4 *colorBuffer,		// 7
						 global uint *lightIndices,			// 8
						 global float *lightLotteryTickets, // 9
						 global float3 *textureBuffer,		// 10
						 global TextureInfo *textureInfo,   // 11
						 global float3 *skyDome,			// 12
						 global TextureInfo *skyInfo,		// 13
						 global Microfacet *microfacets,	// 14
						 int hasSkyDome,					// 15
						 float lightArea,					// 16
						 int lightCount,					// 17
						 int width,							// 18
						 int height							// 19
)
{
	const int x = get_global_id(0);
	const int y = get_global_id(1);
	const int pixelIdx = x + y * width;
	if (pixelIdx >= (width * height))
		return;

	Ray ray = rays[pixelIdx];
	ray.hit_idx = -1;
	ray.t = 1e34f;

	TraceRay(&ray, triangles, nodes, mNodes, primitiveIndices);
	rays[pixelIdx] = ray;
}

kernel void shade_mf(global Ray *rays,					// 0
					 global Material *materials,		// 1
					 global Triangle *triangles,		// 2
					 global BVHNode *nodes,				// 3
					 global MBVHNode *mNodes,			// 4
					 global uint *primitiveIndices,		// 5
					 global uint *seeds,				// 6
					 global float4 *colorBuffer,		// 7
					 global uint *lightIndices,			// 8
					 global float *lightLotteryTickets, // 9
					 global float3 *textureBuffer,		// 10
					 global TextureInfo *textureInfo,   // 11
					 global float3 *skyDome,			// 12
					 global TextureInfo *skyInfo,		// 13
					 global Microfacet *microfacets,	// 14
					 int hasSkyDome,					// 15
					 float lightArea,					// 16
					 int lightCount,					// 17
					 int width,							// 18
					 int height,						// 19
					 int frame							// 20
)
{
	const int x = get_global_id(0);
	const int y = get_global_id(1);
	const int pixelIdx = x + y * width;
	if (pixelIdx >= (width * height))
		return;

	Ray r = rays[pixelIdx];

	// Invalid ray
	if (r.hit_idx < 0)
	{
		float3 color = (float3)(0);
		if (hasSkyDome)
		{
			const float u = (1.0f + atan2(r.direction.x, -r.direction.z) / PI) / 2.0f;
			const float v = 1.0f - acos(r.direction.y) / PI;
			const uint px = (u * (skyInfo->width - 1));
			const uint py = (v * (skyInfo->height - 1));
			const uint idx = px + py * skyInfo->width;
			color = r.throughput * skyDome[idx];
		}

		colorBuffer[pixelIdx] += (float4)(color, 1.0f);
		rays[pixelIdx] = r;
		return;
	}

	uint seed = (pixelIdx + frame * 147565741) * 720898027;

	Triangle t = triangles[r.hit_idx];
	float3 hitpoint = r.origin + r.t * r.direction;
	Material mat = materials[t.mat_idx];

	if (mat.flags > 0)
	{
		if (r.bounces <= 0)
			colorBuffer[pixelIdx] += (float4)(mat.emission, 1.0f);
		else
			colorBuffer[pixelIdx] += (float4)(r.throughput * mat.emission, 1.0f);

		r.hit_idx = -1;
		rays[pixelIdx] = r;
		return;
	}

	const float3 bary = GetBaryCentricCoordinatesTriangleLocal(hitpoint, t);
	float3 normal = normalize(bary.x * t.n0 + bary.y * t.n1 + bary.z * t.n2);

	const float2 t0 = (float2)(t.t0x, t.t0y);
	const float2 t1 = (float2)(t.t1x, t.t1y);
	const float2 t2 = (float2)(t.t2x, t.t2y);
	const float2 texCoords = bary.x * t0 + bary.y * t1 + bary.z * t2;
	const float3 diffuseColor = GetDiffuseColor(mat, textureBuffer, textureInfo, texCoords);

	const int flipNormal = dot(normal, r.direction) > 0.f ? 1 : 0;
	normal = normal * (1.0f - 2.0f * (float)flipNormal);

	r.origin = hitpoint;
	r.throughput *= diffuseColor;

	// refraction
	if (mat.transparency > 0.0f)
	{
		float3 absorption;
		r.direction = Refract(flipNormal, mat, r.direction, normal, &seed, &absorption, r.t);
		r.throughput *= absorption;
	}
	else if (mat.diffuse_intensity < 1.0f)
	{
		const float r1 = RandomFloat(&seed);
		if (r1 > mat.diffuse_intensity) // reflection
		{
			r.direction = Reflect(r.direction, normal);
		}
		else // diffuse
		{
			r.direction = normalize(DiffuseReflectionCosWeighted(normal, seed));
			r.throughput *= PI;
		}
	}
	else // diffuse
	{
		r.direction = normalize(DiffuseReflectionCosWeighted(normal, seed));
		r.throughput *= PI;
	}

	r.origin += EPSILON * r.direction;
	r.bounces++;

//	if (r.bounces >= 3)
//	{
//		const float r1 = RandomFloat(&seed);
//		const float probability = fmax(r.throughput.x, fmax(r.throughput.y, r.throughput.z));
//		if (probability <= 0 || probability < r1 || r.bounces >= MAX_DEPTH)
//			r.hit_idx = -1;
//		else
//			r.throughput /= probability;
//	}

	rays[pixelIdx] = r;
}

kernel void draw_mf(write_only image2d_t outimg, global float4 *previousColor, global float4 *colorBuffer, int samples,
					int width, int height)
{
	const int x = get_global_id(0);
	const int y = get_global_id(1);
	if (x >= width || y >= height)
		return;

	const int pixelIdx = x + y * width;

	const float4 color = colorBuffer[pixelIdx];

	if (color.w > 0.0f)
	{
		const float exponent = 1.0f / 2.2f;
		const float3 temp = color.xyz / colorBuffer[pixelIdx].w;
		const float3 col = pow(temp, (float3)(exponent));

		write_imagef(outimg, (int2)(x, y), (float4)(col, 1.f));
	}
}
