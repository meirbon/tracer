#ifndef MATERIAL_CL
#define MATERIAL_CL

typedef struct
{
	int width, height, offset, dummy;
} TextureInfo;

typedef struct
{
	union {
		union {
			float3 albedo;
			float3 emission;
		};
		struct
		{
			float diffuseR, diffuseG, diffuseB; // 12
			float diffuse_intensity;			// 16
		};
	};
	float absorptionR, absorptionG, absorptionB; // 28
	float refractionIdx;						 // 32

	int diffuseIdx;   // 36
	uint type;		  // 40
	int normalTex;	// 44
	int maskTex;	  // 48
	int displaceTex;  // 52
	float roughnessX; // 56
	float roughnessY; // 60
	float d2;		  // 64
} Material;

float3 getColor(Material mat, global float4 *textures, global TextureInfo *texInfo, float2 texCoords);
float4 getTexValue(int texIdx, global float4 *textures, global TextureInfo *texInfo, float2 texCoords);
float3 getNormal(int3 idx, float3 bary, Material mat, global float4 *textures, global TextureInfo *texInfo,
				 float2 texCoords, global float3 *normals);

inline float3 getColor(Material mat, global float4 *textures, global TextureInfo *texInfo, float2 texCoords)
{
	if (mat.diffuseIdx > -1)
		return getTexValue(mat.diffuseIdx, textures, texInfo, texCoords).xyz;
	else
		return mat.albedo;
}

inline float4 getTexValue(int texIdx, global float4 *textures, global TextureInfo *texInfo, float2 texCoords)
{
	const TextureInfo tInfo = texInfo[texIdx];
	float x = fmod(texCoords.x, 1.0f);
	float y = fmod(texCoords.y, 1.0f);

	if (x < 0)
		x += 1.0f;
	if (y < 0)
		y += 1.0f;

	const uint ix = x * (tInfo.width - 1);
	const uint iy = y * (tInfo.height - 1);

	const uint offset = tInfo.offset + (ix + iy * tInfo.width);

	return textures[offset];
}

inline float3 getNormal(int3 idx, float3 bary, Material mat, global float4 *textures, global TextureInfo *texInfo,
						float2 texCoords, global float3 *normals)
{
	if (mat.normalTex >= 0)
		return normalize(getTexValue(mat.normalTex, textures, texInfo, texCoords).xyz);
	else
		return normalize(t_getNormal(bary, normals[idx.x], normals[idx.y], normals[idx.z]));
}
#endif