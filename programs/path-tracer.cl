#ifndef PATH_TRACER_H
#define PATH_TRACER_H

float3 SampleReference(global Ray *ray, global Material *materials, global Triangle *triangles, global BVHNode *nodes,
                       global MBVHNode *mNodes, global uint *primitiveIndices, global float3 *textureBuffer,
                       global TextureInfo *textureInfo, global float3 *skyDome, global TextureInfo *skyInfo,
                       int hasSkyDome, uint *seed);

float3 SampleNEE_MIS(global Ray *ray, global Material *materials, global Triangle *triangles, global BVHNode *nodes,
                     global MBVHNode *mNodes, global uint *primitiveIndices, global uint *lightIndices,
                     global float *lightLotteryTickets, global float3 *textureBuffer, global TextureInfo *textureInfo,
                     global float3 *skyDome, global TextureInfo *skyInfo, int hasSkyDome, float lightArea,
                     int lightCount, uint *seed);

float3 SampleMicrofacet(global Ray *ray, global Material *materials, global Triangle *triangles, global BVHNode *nodes,
                        global MBVHNode *mNodes, global uint *primitiveIndices, global uint *lightIndices,
                        global float *lightLotteryTickets, global float3 *textureBuffer,
                        global TextureInfo *textureInfo, global float3 *skyDome, global TextureInfo *skyInfo,
                        global Microfacet *microfacets, int hasSkyDome, float lightArea, int lightCount, uint *seed);

inline float3 SampleReference(global Ray *ray, global Material *materials, global Triangle *triangles,
                              global BVHNode *nodes, global MBVHNode *mNodes, global uint *primitiveIndices,
                              global float3 *textureBuffer, global TextureInfo *textureInfo, global float3 *skyDome,
                              global TextureInfo *skyInfo, int hasSkyDome, uint *seed)
{
    float3 E = (float3)(0, 0, 0);
    float3 throughput;
    float PDF = 1.0f / (2.0f * PI);
    Ray r;

    for (int si = 0; si < SAMPLE_COUNT; si++)
    {
        throughput = (float3)(1, 1, 1);
        r = *ray;

        for (uint depth = 0; depth < MAX_DEPTH; depth++)
        {
            r.t = 1e34f;
            r.hit_idx = -1;
            TraceRay(&r, triangles, nodes, mNodes, primitiveIndices);
            if (r.hit_idx < 0)
            {
                if (hasSkyDome)
                {
                    const float u = (1.0f + atan2(r.direction.x, -r.direction.z) / PI) / 2.0f;
                    const float v = 1.0f - acos(r.direction.y) / PI;
                    const uint px = (u * (skyInfo->width - 1));
                    const uint py = (v * (skyInfo->height - 1));
                    const uint idx = px + py * skyInfo->width;
                    E += throughput * skyDome[idx];
                }
                break;
            }

            Triangle t = triangles[r.hit_idx];
            float3 hitPoint = r.origin + r.t * r.direction;
            Material mat = materials[t.mat_idx];

            if (mat.flags > 0)
            { // mat is a light
                E += throughput * mat.emission;
                break;
            }

            const float3 bary = GetBaryCentricCoordinatesTriangleLocal(hitPoint, t);
            float3 normal = normalize(bary.x * t.n0 + bary.y * t.n1 + bary.z * t.n2);

            float2 t0 = (float2)(t.t0x, t.t0y);
            float2 t1 = (float2)(t.t1x, t.t1y);
            float2 t2 = (float2)(t.t2x, t.t2y);
            float2 texCoords = bary.x * t0 + bary.y * t1 + bary.z * t2;
            float3 diffuseColor = GetDiffuseColor(mat, textureBuffer, textureInfo, texCoords);

            int flipNormal = dot(normal, r.direction) > 0.f ? 1 : 0;
            normal = normal * (1.0f - 2.0f * (float)flipNormal);

            r.origin = hitPoint;

            // refraction
            if (mat.transparency > 0.0f)
            {
                float3 absorption;
                r.direction = normalize(Refract(flipNormal, mat, r.direction, normal, seed, &absorption, r.t));
                r.origin += EPSILON * r.direction;
                throughput *= diffuseColor * absorption;
                continue;
            }
            else if (mat.diffuse_intensity < 1.0f) // reflection
            {
                float lottery = RandomFloat(seed);
                if (lottery > mat.diffuse_intensity)
                {
                    r.direction = normalize(Reflect(r.direction, normal));
                    r.origin += EPSILON * r.direction;
                    throughput *= diffuseColor;
                    continue;
                }
            }

            // diffuse
            r.direction = normalize(DiffuseReflection(normal, seed));
            throughput *= (diffuseColor * INVPI) * dot(r.direction, normal) / PDF;
            r.origin += EPSILON * r.direction;
        }
    }

    return E;
}

float3 SampleNEE_MIS(global Ray *ray, global Material *materials, global Triangle *triangles, global BVHNode *nodes,
                     global MBVHNode *mNodes, global uint *primitiveIndices, global uint *lightIndices,
                     global float *lightLotteryTickets, global float3 *textureBuffer, global TextureInfo *textureInfo,
                     global float3 *skyDome, global TextureInfo *skyInfo, int hasSkyDome, float lightArea,
                     int lightCount, uint *seed)
{
    float3 E = (float3)(0, 0, 0), normal;
    float3 throughput, tUpdate, BRDF;
    float PDF;
    Ray r;

    for (int si = 0; si < SAMPLE_COUNT; si++)
    {
        r = *ray;
        throughput = tUpdate = (float3)(1, 1, 1);
        int specular = 1;

        for (uint depth = 0; depth < MAX_DEPTH; depth++)
        {
            r.t = 1e34f;
            r.hit_idx = -1;
            TraceRay(&r, triangles, nodes, mNodes, primitiveIndices);
            if (r.hit_idx < 0)
            {
                if (hasSkyDome)
                {
                    const float u = (1.0f + atan2(r.direction.x, -r.direction.z) / PI) / 2.0f;
                    const float v = 1.0f - acos(r.direction.y) / PI;
                    const uint px = (u * (skyInfo->width - 1));
                    const uint py = (v * (skyInfo->height - 1));
                    const uint idx = px + py * skyInfo->width;
                    E += throughput * skyDome[idx] * tUpdate;
                }
                break;
            }

            Triangle t = triangles[r.hit_idx];
            Material mat = materials[t.mat_idx];
            float3 hitPoint = r.origin + r.t * r.direction;

            if (mat.flags == 1)
            { // mat is a light
                if (depth <= 0)
                {
                    E += throughput * mat.emission;
                }
                else if (specular)
                {
                    E += throughput * mat.emission * tUpdate;
                }
                else
                {
                    const float squaredDistance = r.t * r.t;
                    float3 lightNormal = GetTriangleNormalLocal(hitPoint, t);
                    if (dot(lightNormal, r.direction) > 0.0f)
                        lightNormal *= -1.0f;

                    const float NdotL = dot(normal, r.direction);
                    const float LNdotL = dot(lightNormal, -r.direction);
                    const float SolidAngle = LNdotL * t.m_Area / squaredDistance;
                    const float InversePDFnee = lightArea / t.m_Area;
                    const float lightPDF = 1.0f / SolidAngle;
                    const float brdfPDF = NdotL / PI;

                    if (lightPDF > 0.0f && brdfPDF > 0.0f)
                    {
                        float3 Ld = BRDF * mat.emission * NdotL * InversePDFnee;

                        const float w1 = brdfPDF / (brdfPDF + lightPDF);
                        const float w2 = lightPDF / (brdfPDF + lightPDF);
                        E += throughput * Ld / (w1 * brdfPDF + w2 * lightPDF);
                    }
                }
                break;
            }

            throughput *= tUpdate;
            tUpdate = (float3)(1, 1, 1);

            const float3 bary = GetBaryCentricCoordinatesTriangleLocal(hitPoint, t);
            normal = normalize(bary.x * t.n0 + bary.y * t.n1 + bary.z * t.n2);

            const float2 t0 = (float2)(t.t0x, t.t0y);
            const float2 t1 = (float2)(t.t1x, t.t1y);
            const float2 t2 = (float2)(t.t2x, t.t2y);
            const float2 texCoords = bary.x * t0 + bary.y * t1 + bary.z * t2;
            float3 diffuseColor = GetDiffuseColor(mat, textureBuffer, textureInfo, texCoords);

            BRDF = mat.diffuse * INVPI;

            const int flipNormal = dot(normal, r.direction) > 0.f ? 1 : 0;
            normal = normal * (1.0f - 2.0f * (float)flipNormal);
            r.origin = hitPoint;

            // refraction
            if (mat.transparency > 0.0f)
            {
                float3 absorption;
                r.direction = normalize(Refract(flipNormal, mat, r.direction, normal, seed, &absorption, r.t));
                r.origin += EPSILON * r.direction;
                throughput *= diffuseColor * absorption;
                specular = 1;
                continue;
            }
            else if (mat.diffuse_intensity < 1.0f) // reflection
            {
                float lottery = RandomFloat(seed);
                if (lottery > mat.diffuse_intensity)
                {
                    r.direction = normalize(Reflect(r.direction, normal));
                    r.origin += EPSILON * r.direction;
                    throughput = throughput * diffuseColor;
                    specular = 1;
                    continue;
                }
            }

            // diffuse
            if (lightCount > 0)
            {
                float randomf = RandomFloat(seed);
                float previous = 0.f;
                int winningIdx = 0;
                for (int i = 0; i < lightCount; i++)
                {
                    if (lightLotteryTickets[i] > previous && randomf <= lightLotteryTickets[i])
                    {
                        winningIdx = i;
                        break;
                    }
                    previous = lightLotteryTickets[i];
                }

                Triangle triangle = triangles[lightIndices[winningIdx]];
                Material material = materials[triangle.mat_idx];

                const float3 RandomPointOnLight = RandomPointOnTriangleLocal(triangle, seed);
                const float3 lightNormal = GetTriangleNormalLocal(RandomPointOnLight, triangle);
                float3 L = RandomPointOnLight - hitPoint;
                const float squaredDistance = dot(L, L);
                L /= sqrt(squaredDistance);

                const float NdotL = dot(normal, L);
                const float LNdotL = dot(lightNormal, -1.0f * L);

                if (NdotL > 0.f && LNdotL > 0.f)
                {
                    r.origin = hitPoint + EPSILON * L;
                    r.direction = L;
                    r.t = 1e34f;
                    r.hit_idx = -1;
                    TraceRay(&r, triangles, nodes, mNodes, primitiveIndices);
                    if (r.hit_idx == lightIndices[winningIdx])
                    {
                        const float SolidAngle = LNdotL * triangle.m_Area / squaredDistance;
                        const float3 lightColor = material.emission;
                        const float3 Ld = BRDF * lightColor * NdotL * lightArea / triangle.m_Area;

                        const float bPDF = NdotL / PI;
                        const float lightPDF = 1.0f / SolidAngle;

                        if (lightPDF > 0.0f && bPDF > 0.0f)
                        {
                            const float w1 = bPDF / (bPDF + lightPDF);
                            const float w2 = lightPDF / (bPDF + lightPDF);
                            E += throughput * Ld / (w1 * bPDF + w2 * lightPDF);
                        }
                    }
                }
            }

            r.origin = hitPoint;
            r.direction = normalize(DiffuseReflectionCosWeighted(normal, seed));
            r.origin += EPSILON * r.direction;

            specular = 0;
            const float NdotR = dot(normal, r.direction);
            PDF = NdotR / PI;
            if (PDF <= 0.0f)
                break;

            tUpdate *= BRDF * NdotR / PDF;

            if (depth > 3)
            {
                const float random_value = RandomFloat(seed);
                const float probability = max(throughput.x, max(throughput.y, throughput.z));
                if (probability < random_value || probability <= 0.0f)
                    break;
                tUpdate /= probability;
            }
        }
    }

    return E;
}

inline float3 SampleMicrofacet(global Ray *r, global Material *materials, global Triangle *triangles,
                               global BVHNode *nodes, global MBVHNode *mNodes, global uint *primitiveIndices,
                               global uint *lightIndices, global float *lightLotteryTickets,
                               global float3 *textureBuffer, global TextureInfo *textureInfo, global float3 *skyDome,
                               global TextureInfo *skyInfo, global Microfacet *microfacets, int hasSkyDome,
                               float lightArea, int lightCount, uint *seed)
{
    float3 E = (float3)(0, 0, 0);
    float3 throughput;

    for (int si = 0; si < SAMPLE_COUNT; si++)
    {
        throughput = (float3)(1, 1, 1);
        Ray ray = *r;

        for (uint depth = 0; depth < MAX_DEPTH; depth++)
        {
            ray.t = 1e34f;
            ray.hit_idx = -1;
            TraceRay(&ray, triangles, nodes, mNodes, primitiveIndices);
            if (ray.hit_idx < 0)
            {
                if (hasSkyDome)
                {
                    const float u = (1.0f + atan2(ray.direction.x, -ray.direction.z) / PI) / 2.0f;
                    const float v = 1.0f - acos(ray.direction.y) / PI;
                    const uint px = (u * (skyInfo->width - 1));
                    const uint py = (v * (skyInfo->height - 1));
                    const uint idx = px + py * skyInfo->width;
                    E += throughput * skyDome[idx];
                }
                break;
            }

            Triangle t = triangles[ray.hit_idx];
            Material mat = materials[t.mat_idx];
            Microfacet mf = microfacets[t.mat_idx];
            float3 hitPoint = ray.origin + ray.t * ray.direction;

            if (mat.flags > 0)
            { // mat is a light
                E += throughput * (float3)(mat.emission);
                break;
            }

            const float3 bary = GetBaryCentricCoordinatesTriangleLocal(hitPoint, t);
            float3 normal = normalize(bary.x * t.n0 + bary.y * t.n1 + bary.z * t.n2);

            float2 t0 = (float2)(t.t0x, t.t0y);
            float2 t1 = (float2)(t.t1x, t.t1y);
            float2 t2 = (float2)(t.t2x, t.t2y);
            float2 texCoords = bary.x * t0 + bary.y * t1 + bary.z * t2;
            float3 diffuseColor = GetDiffuseColor(mat, textureBuffer, textureInfo, texCoords);

            int flipNormal = dot(normal, ray.direction) > 0.0f ? 1 : 0;
            normal = normal * (1.0f - 2.0f * (float)flipNormal);

            float weight;
            float3 u, v, w, woLocal;
            float3 wiLocal = worldToLocalMicro(normal, ray.direction, &u, &v, &w);
            float3 wmLocal = SampleWM(mf, seed);
            float3 wm = localToWorldMicro(wmLocal, u, v, w);
            ray.origin = hitPoint;

            // refraction
            if (mat.transparency > 0.0f)
            {
                const float n1 = flipNormal ? mat.refractionIdx : 1.0f;
                const float n2 = flipNormal ? 1.0f : mat.refractionIdx;
                const float n = n1 / n2;
                const float cosTheta = dot(wm, -ray.direction);
                const float k = 1.0f - (n * n) * (1.0f - cosTheta * cosTheta);

                if (k > 0.0f)
                {
                    const float a = n1 - n2;
                    const float b = n1 + n2;
                    const float R0 = (a * a) / (b * b);
                    const float c = 1.0f - cosTheta;
                    const float Fr = R0 + (1.0f - R0) * (c * c * c * c * c);

                    if (RandomFloat(seed) > Fr)
                    {
                        float3 absorption = (float3)(1.0f, 1.0f, 1.0f);
                        if (!flipNormal)
                        {
                            absorption = (float3)(exp(-mat.absorptionR * ray.t), exp(-mat.absorptionG * ray.t),
                                                  exp(-mat.absorptionB * ray.t));
                        }
                        throughput *= absorption;
                        woLocal = normalize(n * -wiLocal + wmLocal * (n * cosTheta - sqrt(k)));
                    }
                    else
                    {
                        woLocal = wmLocal * 2.0f * dot(wmLocal, wiLocal) - wiLocal;
                    }
                }
                else
                {
                    woLocal = wmLocal * 2.0f * dot(wmLocal, wiLocal) - wiLocal;
                }
            }
            else
            {
                woLocal = wmLocal * 2.0f * dot(wmLocal, wiLocal) - wiLocal;
            }

            weight = mf_weight(mf, woLocal, wiLocal, wmLocal);
            float3 wo = localToWorldMicro(woLocal, u, v, w);
            throughput *= diffuseColor * weight;
            ray.origin = hitPoint + EPSILON * wo;
            ray.direction = wo;
        }
    }

    return E;
}

#endif