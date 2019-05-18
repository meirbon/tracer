#include "include.cl"

kernel void shade_nee(global Ray *rays,				   // 0
					  global ShadowRay *sRays,		   // 1
					  global Material *materials,	  // 2
					  global int3 *indices,			   // 3
					  global int *lightIndices,		   // 4
					  global float3 *vertices,		   // 5
					  global float3 *cns,			   // 6
					  global float3 *normals,		   // 7
					  global float2 *texCoords,		   // 8
					  global uint *primitiveIndices,   // 9
					  global float4 *colorBuffer,	  // 10
					  global float4 *textureBuffer,	// 11
					  global TextureInfo *textureInfo, // 12
					  global float4 *skyDome,		   // 13
					  global TextureInfo *skyInfo,	 // 14
					  global Microfacet *microfacets,  // 15
					  global int *matIndices,		   // 16
					  int hasSkyDome,				   // 17
					  int lightCount,				   // 18
					  int width,					   // 19
					  int height,					   // 20
					  int frame						   // 21
)
{
	int intx = get_global_id(0);
	int inty = get_global_id(1);
	if (intx >= width || inty >= height)
		return;
	int pixelIdx = intx + inty * width;

	Ray ray = rays[pixelIdx];
	if (!r_valid(ray))
	{
		float3 color = (float3)(0);
		if (hasSkyDome)
		{
			const float u = (1.0f + atan2(ray.direction.x, -ray.direction.z) / PI) / 2.0f;
			const float v = 1.0f - acos(ray.direction.y) / PI;
			const uint px = (u * (skyInfo->width - 1));
			const uint py = (v * (skyInfo->height - 1));
			const uint idx = px + py * skyInfo->width;
			color = skyDome[idx].xyz;
		}

		colorBuffer[pixelIdx] += (float4)(ray.throughput * color, 1.0f);
		ray.hit_idx = -1;
		rays[pixelIdx] = ray;
		return;
	}

	Material mat = materials[matIndices[ray.hit_idx]];
	if (mat.type == 1) // Material is a light
	{
		float3 color = (float)(ray.lastMatType != 2) * ray.throughput * mat.emission;
		colorBuffer[pixelIdx] += (float4)(color, 1.0f);
		ray.hit_idx = -1;
		ray.throughput = (float3)(0, 0, 0);
		rays[pixelIdx] = ray;
		return;
	}

	uint seed = (pixelIdx + frame * 147565741) * 720898027;
	ray.origin += ray.t * ray.direction;
	const int3 idx = indices[ray.hit_idx];
	const float3 cNormal = cns[ray.hit_idx];
	float3 bary = t_getBaryCoords(ray.origin, cNormal, vertices[idx.x], vertices[idx.y], vertices[idx.z]);
	const float2 tCoord = t_getTexCoords(bary, texCoords[idx.x], texCoords[idx.y], texCoords[idx.z]);

	float3 normal = getNormal(idx, bary, mat, textureBuffer, textureInfo, tCoord, normals);
	const bool backFacing = dot(normal, ray.direction) <= 0.0f;
	if (backFacing)
		normal *= -1.0f;
	const float3 color = getColor(mat, textureBuffer, textureInfo, tCoord);

	ray.lastMatType = mat.type;
	switch (mat.type)
	{
	case (1): // light
		break;
	case (2): // Lambert
	{
		ray.direction = r_cos_reflect(normal, &seed);

		const int light = (int)(RandomFloat(&seed) * (lightCount - 1));
		const int lightIdx = lightIndices[light];
		const int3 lvIdx = indices[lightIdx];
		const float3 lightPos = t_getRandomPointOnSurface(vertices[lvIdx.x], vertices[lvIdx.y], vertices[lvIdx.z],
														  RandomFloat(&seed), RandomFloat(&seed));
		float3 L = lightPos - ray.origin;
		const float squaredDistance = dot(L, L);
		const float distance = sqrt(squaredDistance);
		L /= distance;

		const float3 cNormal = cns[lightIdx];
		const float3 baryLight =
			t_getBaryCoords(lightPos, cNormal, vertices[lvIdx.x], vertices[lvIdx.y], vertices[lvIdx.z]);
		const float3 lightNormal = t_getNormal(baryLight, normals[lvIdx.x], normals[lvIdx.y], normals[lvIdx.z]);

		const float NdotL = dot(-normal, L);
		const float LNdotL = dot(lightNormal, -L);

		if (NdotL > 0 && LNdotL > 0)
		{
			const float area = t_getArea(vertices[lvIdx.x], vertices[lvIdx.y], vertices[lvIdx.z]);
			const float solidAngle = LNdotL * area / squaredDistance;
			const float3 emission = materials[matIndices[lightIdx]].emission;
			const float3 shadowCol = color / PI * emission * solidAngle * NdotL * (float)(lightCount);

			sRays[pixelIdx].origin = ray.origin + EPSILON * L;
			sRays[pixelIdx].direction = L;
			sRays[pixelIdx].color = ray.throughput * shadowCol;
			sRays[pixelIdx].t = distance - EPSILON;
			sRays[pixelIdx].hit_idx = -1;
		}

		ray.throughput *= color;
		ray.origin += EPSILON * ray.direction;
		break;
	}
	case (3): // Specular
	{
		ray.direction = r_reflect(ray.direction, normal);
		ray.origin += EPSILON * ray.direction;
		ray.throughput *= color;
		break;
	}
	case (4): // Fresnel
	{
		ray.throughput *= color * r_refract(backFacing, mat, &ray.origin, &ray.direction, normal, &seed, ray.t);
		break;
	}
	default:
		break;
	}

	if (ray.bounces >= 1)
	{
		const float prob = min(1.0f, max(ray.throughput.x, max(ray.throughput.y, ray.throughput.z)));
		if (ray.bounces < MAX_DEPTH && prob > EPSILON && prob > RandomFloat(&seed))
		{
			ray.origin += ray.direction * EPSILON;
			ray.bounces++;
			ray.throughput /= prob;
		}
		else // Stop this ray
		{
			ray.hit_idx = -1;
			ray.throughput = (float3)(0);
			colorBuffer[pixelIdx].w += 1.0f;
		}
	}

	rays[pixelIdx] = ray;
}