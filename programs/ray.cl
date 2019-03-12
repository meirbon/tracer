#ifndef RAY_H
#define RAY_H

typedef struct Ray {
    union {
        float3 origin; // 16
        struct {
            float orgX, orgY, orgZ;
            float t;
        };
    };
    union {
        float3 direction; // 32
        struct {
            float dirX, dirY, dirZ;
            int hit_idx; // 40
        };
    };
    float3 color;
} Ray; // 48

float3 World2Local(float3 V, float3 N);
float3 DiffuseReflectionCosWeighted(float3 normal, uint* seed);
float3 Reflect(float3 A, float3 B);
float3 Refract(int inside, global Material* mat, float3 D, float3 N, uint* seed, float3* absorption, float t);
float3 uniformSampleHemisphere(float r1, float r2);
float3 cosineWeightedSample(float r1, float r2);
void createCoordinateSystem(float3* N, float3* Nt, float3* Nb);
float3 localToWorld(float3 sample, float3 Nt, float3 Nb, float3 normal);
float3 DiffuseReflection(float3 normal, uint* seed);

float3 World2Local(float3 V, float3 N)
{
    float3 tmp = (fabs(N.x) > 0.99f) ? (float3)(0, 1, 0) : (float3)(1, 0, 0);
    float3 B = normalize(cross(N, tmp));
    float3 T = cross(B, N);
    return (float3)(dot(V, T), dot(V, B), dot(V, N));
}

float3 DiffuseReflectionCosWeighted(float3 normal, uint* seed)
{
    // based on SmallVCM
    float3 Nt, Nb;
    createCoordinateSystem(&normal, &Nt, &Nb);
    float r1 = RAND(), r2 = RAND();
    float3 sample = cosineWeightedSample(r1, r2);
    return localToWorld(sample, Nt, Nb, normal);
}

float3 Reflect(float3 A, float3 B)
{
    return A - 2.0f * B * dot(A, B);
}

float3 Refract(int inside, global Material* mat, float3 D, float3 N, uint* seed, float3* absorption, float t)
{
    *absorption = (float3)(1, 1, 1);
    float3 R = Reflect(D, N);
    float n1, n2;
    int doAbsorption = 0;
    if (inside) {
        n1 = mat->refractionIdx;
        n2 = 1.0f;
    } else {
        n1 = 1.0f;
        n2 = mat->refractionIdx;
        doAbsorption = 1;
    }

    float n = n1 / n2;
    float cosTheta = dot(N, -D);
    float k = 1.0f - (n * n) * (1.0f - cosTheta * cosTheta);
    if (k > 0.0f) {
        float a = n1 - n2;
        float b = n1 + n2;
        float R0 = (a * a) / (b * b);
        float minCosTheta = 1.0f - cosTheta;
        float fR = R0 + (1 - R0) * (minCosTheta * minCosTheta * minCosTheta * minCosTheta * minCosTheta);

        float rand1 = RandomFloat(seed);

        if (rand1 > fR) {
            *absorption = (float3)(exp(-mat->absorptionR * t),
                exp(-mat->absorptionG * t),
                exp(-mat->absorptionB * t));
            R = normalize(n * D + N * (n * cosTheta - sqrt(fabs(k))));
        }
    }

    return R;
}

float3 uniformSampleHemisphere(float r1, float r2)
{
    float sinTheta = sqrt(1.f - r1 * r1);
    float phi = 2.0f * PI * r2;
    float x = sinTheta * cos(phi);
    float z = sinTheta * sin(phi);
    return (float3)(x, r1, z);
}

float3 cosineWeightedSample(float r1, float r2)
{
    const float r = sqrt(r1);
    float theta = 2.0f * PI * r2;
    float x = r * cos(theta);
    float z = r * sin(theta);
    return (float3)(x, fmax(0.0f, sqrt(1.0f - r1)), z);
}

void createCoordinateSystem(float3* N, float3* Nt, float3* Nb)
{
    if (fabs(N->x) > fabs(N->y)) {
        *Nt = (float3)(N->z, 0.f, -N->x) / sqrt(N->x * N->x + N->z * N->z);
    } else {
        *Nt = (float3)(0.f, -N->z, N->y) / sqrt(N->y * N->y + N->z * N->z);
    }
    *Nb = cross(*N, *Nt);
}

float3 localToWorld(float3 sample, float3 Nt, float3 Nb, float3 normal)
{
    return (float3)(
        sample.x * Nb.x + sample.y * normal.x + sample.z * Nt.x,
        sample.x * Nb.y + sample.y * normal.y + sample.z * Nt.y,
        sample.x * Nb.z + sample.y * normal.z + sample.z * Nt.z);
}

float3 DiffuseReflection(float3 normal, uint* seed)
{
    float3 Nt, Nb;
    createCoordinateSystem(&normal, &Nt, &Nb);
    float r1 = RAND();
    float r2 = RAND();
    float3 sample = uniformSampleHemisphere(r1, r2);
    return localToWorld(sample, Nt, Nb, normal);
}

#endif