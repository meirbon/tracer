#define PI 3.14159265358979323846f
#define INVPI (1.0f / PI)
#define DEBUG_BVH 0
#define DEBUG_RAND 0
#define SAMPLE_COUNT 1
#define MAX_DEPTH 10
#define VARIANCE_REDUCTION 0
#define MICROFACETS 1

#include "include.cl"

kernel void generateRays(global Ray *rays, global GpuCamera *camera, global uint *seeds, float3 horizontal,
                         float3 vertical, int width, int height)
{
    unsigned int intx = get_global_id(0);
    unsigned int inty = get_global_id(1);
    const unsigned int pixelIdx = intx + inty * width;
    if (pixelIdx >= (width * height))
        return;

    uint seed = seeds[pixelIdx];

    float x = (float)intx + RandomFloat(&seed) - .5f;
    float y = (float)inty + RandomFloat(&seed) - .5f;

    const float PixelX = x * camera->invWidth;
    const float PixelY = y * camera->invHeight;
    const float ScreenX = 2.f * PixelX - 1.f;
    const float ScreenY = 1.f - 2.f * PixelY;

    rays[pixelIdx].origin = camera->origin;
    rays[pixelIdx].direction = normalize(camera->viewDirection + horizontal * ScreenX + vertical * ScreenY);
    rays[pixelIdx].hit_idx = -1;
    rays[pixelIdx].t = 1e34f;
    rays[pixelIdx].color = (float3)(1, 1, 1);

    seeds[pixelIdx] = seed;
}

kernel void intersectRaysMF(global Ray *rays,                  // 0
                            global Material *materials,        // 1
                            global Triangle *triangles,        // 2
                            global BVHNode *nodes,             // 3
                            global MBVHNode *mNodes,           // 4
                            global uint *primitiveIndices,     // 5
                            global uint *seeds,                // 6
                            global float4 *colorBuffer,        // 7
                            global uint *lightIndices,         // 8
                            global float *lightLotteryTickets, // 9
                            global float3 *textureBuffer,      // 10
                            global TextureInfo *textureInfo,   // 11
                            global float3 *skyDome,            // 12
                            global TextureInfo *skyInfo,       // 13
                            global Microfacet *microfacets,    // 14
                            int hasSkyDome,                    // 15
                            float lightArea,                   // 16
                            int lightCount,                    // 17
                            int width,                         // 18
                            int height                         // 19
)
{
    const uint x = get_global_id(0);
    const uint y = get_global_id(1);
    const uint pixelIdx = x + y * width;
    if (pixelIdx >= (width * height))
        return;

    uint seed = seeds[pixelIdx];

    global Ray *ray = &rays[pixelIdx];

    const float3 E = SampleMicrofacet(ray, materials, triangles, nodes, mNodes, primitiveIndices, lightIndices,
                                      lightLotteryTickets, textureBuffer, textureInfo, skyDome, skyInfo, microfacets,
                                      hasSkyDome, lightArea, lightCount, &seed);

    seeds[pixelIdx] = seed; // update seed
    colorBuffer[pixelIdx] = (float4)(E, 1.0f);
}

kernel void intersectRays(global Ray *rays,                  // 0
                          global Material *materials,        // 1
                          global Triangle *triangles,        // 2
                          global BVHNode *nodes,             // 3
                          global MBVHNode *mNodes,           // 4
                          global uint *primitiveIndices,     // 5
                          global uint *seeds,                // 6
                          global float4 *colorBuffer,        // 7
                          global uint *lightIndices,         // 8
                          global float *lightLotteryTickets, // 9
                          global float3 *textureBuffer,      // 10
                          global TextureInfo *textureInfo,   // 11
                          global float3 *skyDome,            // 12
                          global TextureInfo *skyInfo,       // 13
                          global Microfacet *microfacets,    // 14
                          int hasSkyDome,                    // 15
                          float lightArea,                   // 16
                          int lightCount,                    // 17
                          int width,                         // 18
                          int height                         // 19
)
{
    const uint x = get_global_id(0);
    const uint y = get_global_id(1);
    const uint pixelIdx = x + y * width;
    if (pixelIdx >= (width * height))
        return;

    uint seed = seeds[pixelIdx];

    global Ray *ray = &rays[pixelIdx];

    const float3 E = SampleReference(ray, materials, triangles, nodes, mNodes, primitiveIndices, textureBuffer,
                                     textureInfo, skyDome, skyInfo, hasSkyDome, &seed);

    seeds[pixelIdx] = seed; // update seed
    colorBuffer[pixelIdx] = (float4)(E, 1.0f);
}

kernel void intersectRaysOpt(global Ray *rays,                  // 0
                             global Material *materials,        // 1
                             global Triangle *triangles,        // 2
                             global BVHNode *nodes,             // 3
                             global MBVHNode *mNodes,           // 4
                             global uint *primitiveIndices,     // 5
                             global uint *seeds,                // 6
                             global float4 *colorBuffer,        // 7
                             global uint *lightIndices,         // 8
                             global float *lightLotteryTickets, // 9
                             global float3 *textureBuffer,      // 10
                             global TextureInfo *textureInfo,   // 11
                             global float3 *skyDome,            // 12
                             global TextureInfo *skyInfo,       // 13
                             global Microfacet *microfacets,    // 14
                             int hasSkyDome,                    // 15
                             float lightArea,                   // 16
                             int lightCount,                    // 17
                             int width,                         // 18
                             int height                         // 19
)
{
    const uint x = get_global_id(0);
    const uint y = get_global_id(1);
    const uint pixelIdx = x + y * width;
    if (pixelIdx >= (width * height))
        return;

    uint seed = seeds[pixelIdx];

    global Ray *ray = &rays[pixelIdx];

    const float3 E =
        SampleNEE_MIS(ray, materials, triangles, nodes, mNodes, primitiveIndices, lightIndices, lightLotteryTickets,
                      textureBuffer, textureInfo, skyDome, skyInfo, hasSkyDome, lightArea, lightCount, &seed);

    seeds[pixelIdx] = seed; // update seed
    colorBuffer[pixelIdx] = (float4)(E, 1.0f);
}

kernel void intersectRaysBVH(global Ray *rays,                  // 0
                             global Material *materials,        // 1
                             global Triangle *triangles,        // 2
                             global BVHNode *nodes,             // 3
                             global MBVHNode *mNodes,           // 4
                             global uint *primitiveIndices,     // 5
                             global uint *seeds,                // 6
                             global float4 *colorBuffer,        // 7
                             global uint *lightIndices,         // 8
                             global float *lightLotteryTickets, // 9
                             global float3 *textureBuffer,      // 10
                             global TextureInfo *textureInfo,   // 11
                             global float3 *skyDome,            // 12
                             global TextureInfo *skyInfo,       // 13
                             global Microfacet *microfacets,    // 14
                             int hasSkyDome,                    // 15
                             float lightArea,                   // 16
                             int lightCount,                    // 17
                             int width,                         // 18
                             int height                         // 19
)
{
    const uint x = get_global_id(0);
    const uint y = get_global_id(1);
    const uint pixelIdx = x + y * width;
    if (pixelIdx >= (width * height))
        return;

    uint seed = seeds[pixelIdx];

    global Ray *ray = &rays[pixelIdx];

    Trace(ray, triangles, nodes, mNodes, primitiveIndices);
    if (ray->t > 0.0f)
    {
        colorBuffer[pixelIdx] = (float4)(rays[pixelIdx].t / 64.f, 1.0f - rays[pixelIdx].t / 48.f, 0, 1);
    }
    else
    {
        colorBuffer[pixelIdx] = (float4)(0, 0, 0, 1);
    }
}

kernel void Draw(write_only image2d_t outimg, global float4 *previousColor, global float4 *colorBuffer, int samples,
                 int width, int height)
{
    const uint x = get_global_id(0);
    const uint y = get_global_id(1);
    const uint pixelIdx = x + y * width;
    if (pixelIdx >= (width * height))
        return;

#if DEBUG_BVH
    write_imagef(outimg, (int2)(x, y), colorBuffer[pixelIdx]); // write to texture
    return;
#endif

    if (samples > 2)
    {
        float factor = 1.0f / (float)(samples + 1);
        previousColor[pixelIdx] =
            (previousColor[pixelIdx] * factor * samples) + (colorBuffer[pixelIdx] / SAMPLE_COUNT * factor);
    }
    else
    {
        previousColor[pixelIdx] = colorBuffer[pixelIdx] / SAMPLE_COUNT;
    }

    write_imagef(outimg, (int2)(x, y),
                 (float4)(sqrt(previousColor[pixelIdx].x), sqrt(previousColor[pixelIdx].y),
                          sqrt(previousColor[pixelIdx].z), 1.f)); // write to texture
}

kernel void intersect(global Ray *rays,                  // 0
                      global Material *materials,        // 1
                      global Triangle *triangles,        // 2
                      global BVHNode *nodes,             // 3
                      global MBVHNode *mNodes,           // 4
                      global uint *primitiveIndices,     // 5
                      global uint *seeds,                // 6
                      global float4 *colorBuffer,        // 7
                      global uint *lightIndices,         // 8
                      global float *lightLotteryTickets, // 9
                      global float3 *textureBuffer,      // 10
                      global TextureInfo *textureInfo,   // 11
                      global float3 *skyDome,            // 12
                      global TextureInfo *skyInfo,       // 13
                      global Microfacet *microfacets,    // 14
                      int hasSkyDome,                    // 15
                      float lightArea,                   // 16
                      int lightCount,                    // 17
                      int width,                         // 18
                      int height                         // 19
)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    if (x >= width || y >= height)
        return;
    const int pixelIdx = x + y * width;

    Ray ray = rays[pixelIdx];
    if (ray.t < 0.0f)
        return;

    TraceRay(&ray, triangles, nodes, mNodes, primitiveIndices);
    if (ray.hit_idx < 0)
    {
        ray.t = -1.0f;
    }

    rays[pixelIdx] = ray;
}

kernel void shade(global Ray *rays,                  // 0
                  global Material *materials,        // 1
                  global Triangle *triangles,        // 2
                  global BVHNode *nodes,             // 3
                  global MBVHNode *mNodes,           // 4
                  global uint *primitiveIndices,     // 5
                  global uint *seeds,                // 6
                  global float4 *colorBuffer,        // 7
                  global uint *lightIndices,         // 8
                  global float *lightLotteryTickets, // 9
                  global float3 *textureBuffer,      // 10
                  global TextureInfo *textureInfo,   // 11
                  global float3 *skyDome,            // 12
                  global TextureInfo *skyInfo,       // 13
                  global Microfacet *microfacets,    // 14
                  int hasSkyDome,                    // 15
                  float lightArea,                   // 16
                  int lightCount,                    // 17
                  int width,                         // 18
                  int height                         // 19
)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    if (x >= width || y >= height)
        return;

    const int pixelIdx = x + y * width;
    uint seed = seeds[pixelIdx];

    Ray ray = rays[pixelIdx];
    if (ray.t < 0.0f)
    {
        if (hasSkyDome)
        {
            const float u = (1.0f + atan2(ray.dirX, -ray.dirZ) / PI) / 2.0f;
            const float v = 1.0f - acos(ray.dirY) / PI;
            const uint px = (u * (skyInfo->width - 1));
            const uint py = (v * (skyInfo->height - 1));
            const uint idx = px + py * skyInfo->width;
            colorBuffer[pixelIdx] += (float4)(skyDome[idx] * ray.color, 1.0f);
            ray.color = (float3)(0, 0, 0);
            rays[pixelIdx] = ray;
        }
        return;
    }

    Triangle triangle = triangles[ray.hit_idx];
    Material mat = materials[triangle.mat_idx];
    float3 hitPoint = ray.origin + ray.t * ray.direction;

    // In case we hit a light
    if (mat.flags > 0)
    { // mat is a light
        colorBuffer[pixelIdx] = (float4)(ray.color * mat.emission, 1.0f);
        rays[pixelIdx].t = -1.0f;
        return;
    }

    // Calculate triangle normal
    const float3 bary = GetBaryCentricCoordinatesTriangleLocal(hitPoint, triangle);
    float3 normal = normalize(bary.x * triangle.n0 + bary.y * triangle.n1 + bary.z * triangle.n2);

    // Get texture coordinates
    float2 texCoords = bary.x * (float2)(triangle.t0x, triangle.t0y);
    texCoords += bary.y * (float2)(triangle.t1x, triangle.t1y);
    texCoords += bary.z * (float2)(triangle.t2x, triangle.t2y);

    // Set new origin and direction
    ray.origin = hitPoint + EPSILON * normal;
    ray.direction = normalize(DiffuseReflection(normal, seed));

    // Update throughput
    ray.color *= GetDiffuseColor(mat, textureBuffer, textureInfo, texCoords);
    ray.color *= INVPI * dot(ray.direction, normal);
    ray.color *= 2.0f * PI;

    // Reset ray
    ray.t = 1e34f;
    ray.hit_idx = -1;

    rays[pixelIdx] = ray;
    seeds[pixelIdx] = seed;
}

kernel void draw(write_only image2d_t outimg, global Ray *rays, global float4 *previousColor,
                 global float4 *colorBuffer, int samples, int width, int height)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const int pixelIdx = x + y * width;
    if (pixelIdx >= (width * height))
        return;

    float4 color = colorBuffer[pixelIdx];

    if (samples > 2)
    {
        float factor = 1.0f / (float)(samples + 1);
        previousColor[pixelIdx] = (previousColor[pixelIdx] * factor * samples) + (color / SAMPLE_COUNT * factor);
    }
    else
    {
        //        previousColor[pixelIdx] = (float4)(color / SAMPLE_COUNT, 1.0f);
        previousColor[pixelIdx] = (color / SAMPLE_COUNT, 1.0f);
    }

    colorBuffer[pixelIdx] = (float4)(0, 0, 0, 0);
    write_imagef(outimg, (int2)(x, y),
                 (float4)(sqrt(previousColor[pixelIdx].x), sqrt(previousColor[pixelIdx].y),
                          sqrt(previousColor[pixelIdx].z), 1.f)); // write to texture
}