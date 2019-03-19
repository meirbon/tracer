#ifndef MATERIAL_H
#define MATERIAL_H

typedef struct TextureInfo
{
    int width, height, offset, dummy;
} TextureInfo;

typedef struct Material
{
    union {
        float3 diffuse;
        struct
        {
            float diffuseR, diffuseG, diffuseB; // 12
            float diffuse_intensity;            // 16
        };
    };
    union {
        float3 emission;
        struct
        {
            float emissionR, emissionG, emissionB; // 28
            float roughness;                       // 32
        };
    };
    float absorptionR, absorptionG, absorptionB; // 44
    float refractionIdx;                         // 48

    float transparency; // 52
    int textureIdx;     // 56
    uint flags;         // 60
    float dummy;
} Material;

float3 GetDiffuseColor(Material mat, global float3 *textures, global TextureInfo *texInfo, float2 texCoords);

inline float3 GetDiffuseColor(Material mat, global float3 *textures, global TextureInfo *texInfo,
                              float2 texCoords)
{
    if (mat.textureIdx > -1)
    {
        const uint x = min(texInfo->width, max(0, (int)(texCoords.x * texInfo[mat.textureIdx].width) - 1));
        const uint y = min(texInfo->height, max(0, (int)(texCoords.y * texInfo[mat.textureIdx].height) - 1));
        const uint idx = texInfo[mat.textureIdx].offset + y * texInfo[mat.textureIdx].width + x;
        return textures[idx];
    }
    else
    {
        return mat.diffuse;
    }
}

#endif