#ifndef RAY_CL
#define RAY_CL

typedef struct
{
	float3 origin;
	float3 direction;
	float3 throughput;
	float3 lastNormal;

	float t;
	int hit_idx;
	int index;
	int bounces;
} Ray;

bool r_valid(Ray r);
float3 r_reflect(float3 dir, float3 normal);
float3 r_refract(int inside, Material mat, float3 *dir, float3 N, uint *seed, float t);
float3 r_diffuse_reflect(float3 normal, uint *seed);
float3 sampleHemi(float r1, float r2);
float3 sampleCos(float r1, float r2);
void worldToLocal(float3 N, float3 *Nt, float3 *Nb);
float3 localToWorld(float3 sample, float3 Nt, float3 Nb, float3 normal);

inline bool r_valid(Ray r) { return r.t < 1e34f; }

float3 r_reflect(float3 dir, float3 normal) { return dir - 2.0f * normal * dot(dir, normal); }

inline float3 r_refract(int inside, Material mat, float3 *dir, float3 N, uint *seed, float t)
{
	float3 absorption = (float3)(1, 1, 1);
	float3 D = *dir;
	*dir = r_reflect(D, N);

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
				absorption = (float3)(exp(-mat.absorptionR * t), exp(-mat.absorptionG * t), exp(-mat.absorptionB * t));
			*dir = normalize(n * D + N * (n * cosTheta - sqrt(k)));
		}
	}

	return absorption;
}

float3 r_diffuse_reflect(float3 normal, uint *seed)
{
	float3 Nt, Nb;
	worldToLocal(normal, &Nt, &Nb);
	float3 sample = sampleHemi(RandomFloat(seed), RandomFloat(seed));
	return normalize(localToWorld(sample, Nt, Nb, normal));
}

float3 sampleHemi(float r1, float r2)
{
	float r = sqrt(1.f - r1 * r1);
	float phi = 2.0f * PI * r2;
	float x = r * cos(phi);
	float y = r * sin(phi);
	return (float3)(x, y, r1);
}

float3 sampleCos(float r1, float r2)
{
	const float r = sqrt(r1);
	const float theta = 2.0f * PI * r2;
	const float x = r * cos(theta);
	const float y = r * sin(theta);
	return normalize((float3)(x, y, fmax(0.0f, sqrt(1.0f - r1))));
}

void worldToLocal(float3 N, float3 *Nt, float3 *Nb)
{
	const float3 w = fabs(N.x) > 0.99f ? ((float3)(0.0f, 1.0f, 0.0f)) : ((float3)(1.0f, 0.0f, 0.0f));
	*Nt = normalize(cross(N, w));
	*Nb = cross(N, *Nt);
}

float3 localToWorld(float3 sample, float3 Nt, float3 Nb, float3 normal)
{
	return sample.x * Nt + sample.y * Nb + sample.z * normal;
}
#endif