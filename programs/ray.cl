#pragma once

typedef struct Ray
{
	union {
		float3 origin; // 16
		struct
		{
			float orgX, orgY, orgZ;
			float t;
		};
	};
	union {
		float3 direction; // 32
		struct
		{
			float dirX, dirY, dirZ;
			int hit_idx;
		};
	};

	float3 throughput; // 64
	float3 lastNormal; // 80
	int bounces;	   // 84
} Ray;				   // 48

float3 DiffuseReflectionCosWeighted(float3 normal, uint *seed);
float3 Reflect(float3 A, float3 B);
float3 Refract(int inside, Material mat, float3 D, float3 N, uint *seed, float3 *absorption, float t);
float3 uniformSampleHemisphere(float r1, float r2);
float3 cosineWeightedSample(float r1, float r2);
void createCoordinateSystem(float3 N, float3 *Nt, float3 *Nb);
float3 localToWorld(float3 sample, float3 Nt, float3 Nb, float3 normal);
float3 DiffuseReflection(float3 normal, uint *seed);

float3 DiffuseReflectionCosWeighted(float3 normal, uint *seed)
{
	// based on SmallVCM
	float3 Nt, Nb;
	createCoordinateSystem(normal, &Nt, &Nb);
	float r1 = RandomFloat(seed), r2 = RandomFloat(seed);
	float3 sample = cosineWeightedSample(r1, r2);
	return localToWorld(sample, Nt, Nb, normal);
}

float3 Reflect(float3 A, float3 B) { return A - 2.0f * B * dot(A, B); }

inline float3 Refract(int inside, Material mat, float3 D, float3 N, uint *seed, float3 *absorption, float t)
{
	*absorption = (float3)(1, 1, 1);
	float3 R = Reflect(D, N);

	const float n1 = inside ? mat.refractionIdx : 1.0f;
	const float n2 = inside ? 1.0f : mat.refractionIdx;
	const float n = n1 / n2;
	const float cosTheta = dot(N, -D);
	const float k = 1.0f - (n * n) * (1.0f - cosTheta * cosTheta);

	if (k > 0.0f)
	{
		float a = n1 - n2;
		float b = n1 + n2;
		float R0 = (a * a) / (b * b);
		float c = 1.0f - cosTheta;
		float fR = R0 + (1.0f - R0) * (c * c * c * c * c);

		const float rand1 = RandomFloat(seed);

		if (rand1 > fR)
		{
			if (!inside)
				*absorption = (float3)(exp(-mat.absorptionR * t), exp(-mat.absorptionG * t), exp(-mat.absorptionB * t));
			R = normalize(n * D + N * (n * cosTheta - sqrt(k)));
		}
	}

	return R;
}

float3 uniformSampleHemisphere(float r1, float r2)
{
	float r = sqrt(1.f - r1 * r1);
	float phi = 2.0f * PI * r2;
	float x = r * cos(phi);
	float y = r * sin(phi);
	return (float3)(x, y, r1);
}

float3 cosineWeightedSample(float r1, float r2)
{
	const float r = sqrt(r1);
	const float theta = 2.0f * PI * r2;
	const float x = r * cos(theta);
	const float y = r * sin(theta);
	return normalize((float3)(x, y, fmax(0.0f, sqrt(1.0f - r1))));
}

void createCoordinateSystem(float3 N, float3 *Nt, float3 *Nb)
{
	const float3 w = fabs(N.x) > 0.99f ? ((float3)(0.0f, 1.0f, 0.0f)) : ((float3)(1.0f, 0.0f, 0.0f));
	*Nt = normalize(cross(N, w));
	*Nb = cross(N, *Nt);
}

float3 localToWorld(float3 sample, float3 Nt, float3 Nb, float3 normal)
{
	return sample.x * Nt + sample.y * Nb + sample.z * normal;
}

float3 DiffuseReflection(float3 normal, uint *seed)
{
	float3 Nt, Nb;
	createCoordinateSystem(normal, &Nt, &Nb);
	float3 sample = uniformSampleHemisphere(RandomFloat(seed), RandomFloat(seed));
	return normalize(localToWorld(sample, Nt, Nb, normal));
}
