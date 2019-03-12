#ifndef SPHERE_H
#define SPHERE_H

void IntersectSphere(global Ray* ray);

void IntersectSphere(global Ray* ray)
{
	const float3 center = (float3)(0.f, 0.f, -3.f);

	const float3 rPos = ray->origin - center;

	const float a = dot(ray->direction, ray->direction);
	const float b = dot(ray->direction * 2.f, rPos);
	const float c = dot(rPos, rPos) - .5f;

	const float discriminant = (b * b) - (4.f * a * c);

	if (discriminant >= 0.f) {
		const float div2a = 1.f / (2.f * a);
		const float sqrtDis = discriminant == 0.f ? 0.f : sqrt(discriminant);
		const float t1 = ((-b) + sqrtDis) * div2a;
		const float t2 = ((-b) - sqrtDis) * div2a;

		const float tSlow = t1 > 0.f && t1 < t2 ? t1 : t2; //Take the largest one, one should be positive...
		if (tSlow > 0.f && tSlow < ray->t) {
			ray->t = tSlow;
		}
	}
}

#endif