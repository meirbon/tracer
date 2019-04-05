#ifndef MBVH_H
#define MBVH_H

typedef struct MBVHNode
{
	float4 minx; // 16
	float4 maxx; // 32

	float4 miny; // 48
	float4 maxy; // 64

	float4 minz; // 80
	float4 maxz; // 96

	int child[4]; // 112
	int count[4]; // 128
} MBVHNode;

typedef struct MBVHTraversal
{
	int leftFirst; // Node
	int count;
} MBVHTraversal;

typedef struct MBVHHit
{
	union {
		float4 tmin4;
		float tmin[4];
		int tmini[4];
	};
	uint result[4];
} MBVHHit;

MBVHHit IntersectMBVHNode(global MBVHNode *node, global Ray *ray);
MBVHHit IntersectMBVHNodeRay(global MBVHNode *node, Ray *ray);
void IntersectMBVHTree(global MBVHNode *nodes, global Triangle *triangles, global uint *primitiveIndices,
					   global Ray *ray);
void IntersectMBVHTreeRay(global MBVHNode *nodes, global Triangle *triangles, global uint *primitiveIndices, Ray *ray);

inline MBVHHit IntersectMBVHNode(global MBVHNode *node, global Ray *ray)
{
	MBVHHit hit;
	float3 invDir = 1.f / ray->direction;

	// X axis
	float4 org = (float4)(ray->origin.x, ray->origin.x, ray->origin.x, ray->origin.x);
	float4 dir = (float4)(invDir.x, invDir.x, invDir.x, invDir.x);
	float4 t1 = (node->minx - org) * dir;
	float4 t2 = (node->maxx - org) * dir;

	hit.tmin4 = fmin(t1, t2);
	float4 tmax = fmax(t1, t2);

	// Y axis
	org = (float4)(ray->origin.y, ray->origin.y, ray->origin.y, ray->origin.y);
	dir = (float4)(invDir.y, invDir.y, invDir.y, invDir.y);
	t1 = (node->miny - org) * dir;
	t2 = (node->maxy - org) * dir;

	hit.tmin4 = fmax(hit.tmin4, fmin(t1, t2));
	tmax = fmin(tmax, fmax(t1, t2));

	// Z axis
	org = (float4)(ray->origin.z, ray->origin.z, ray->origin.z, ray->origin.z);
	dir = (float4)(invDir.z, invDir.z, invDir.z, invDir.z);
	t1 = (node->minz - org) * dir;
	t2 = (node->maxz - org) * dir;

	hit.tmin4 = fmax(hit.tmin4, fmin(t1, t2));
	tmax = fmin(tmax, fmax(t1, t2));

	hit.tmini[0] = ((hit.tmini[0] & 0xFFFFFFFC) | 0b00);
	hit.tmini[1] = ((hit.tmini[1] & 0xFFFFFFFC) | 0b01);
	hit.tmini[2] = ((hit.tmini[2] & 0xFFFFFFFC) | 0b10);
	hit.tmini[3] = ((hit.tmini[3] & 0xFFFFFFFC) | 0b11);

	hit.result[0] = ((tmax.x > 0) && (hit.tmin4.x <= tmax.x) && hit.tmin4.x < ray->t) ? 1 : 0;
	hit.result[1] = ((tmax.y > 0) && (hit.tmin4.y <= tmax.y) && hit.tmin4.y < ray->t) ? 1 : 0;
	hit.result[2] = ((tmax.z > 0) && (hit.tmin4.z <= tmax.z) && hit.tmin4.z < ray->t) ? 1 : 0;
	hit.result[3] = ((tmax.w > 0) && (hit.tmin4.w <= tmax.w) && hit.tmin4.w < ray->t) ? 1 : 0;

	float tmp;
	if (hit.tmin[0] > hit.tmin[1])
	{
		tmp = hit.tmin[0];
		hit.tmin[0] = hit.tmin[1];
		hit.tmin[1] = tmp;
	}
	if (hit.tmin[2] > hit.tmin[3])
	{
		tmp = hit.tmin[2];
		hit.tmin[2] = hit.tmin[3];
		hit.tmin[3] = tmp;
	}
	if (hit.tmin[0] > hit.tmin[2])
	{
		tmp = hit.tmin[0];
		hit.tmin[0] = hit.tmin[2];
		hit.tmin[2] = tmp;
	}
	if (hit.tmin[1] > hit.tmin[3])
	{
		tmp = hit.tmin[1];
		hit.tmin[1] = hit.tmin[3];
		hit.tmin[3] = tmp;
	}
	if (hit.tmin[2] > hit.tmin[3])
	{
		tmp = hit.tmin[2];
		hit.tmin[2] = hit.tmin[3];
		hit.tmin[3] = tmp;
	}

	return hit;
}

inline MBVHHit IntersectMBVHNodeRay(global MBVHNode *node, Ray *ray)
{
	MBVHHit hit;
	float3 invDir = 1.f / ray->direction;

	// X axis
	float4 org = (float4)(ray->origin.x, ray->origin.x, ray->origin.x, ray->origin.x);
	float4 dir = (float4)(invDir.x, invDir.x, invDir.x, invDir.x);
	float4 t1 = (node->minx - org) * dir;
	float4 t2 = (node->maxx - org) * dir;

	hit.tmin4 = fmin(t1, t2);
	float4 tmax = fmax(t1, t2);

	// Y axis
	org = (float4)(ray->origin.y, ray->origin.y, ray->origin.y, ray->origin.y);
	dir = (float4)(invDir.y, invDir.y, invDir.y, invDir.y);
	t1 = (node->miny - org) * dir;
	t2 = (node->maxy - org) * dir;

	hit.tmin4 = fmax(hit.tmin4, fmin(t1, t2));
	tmax = fmin(tmax, fmax(t1, t2));

	// Z axis
	org = (float4)(ray->origin.z, ray->origin.z, ray->origin.z, ray->origin.z);
	dir = (float4)(invDir.z, invDir.z, invDir.z, invDir.z);
	t1 = (node->minz - org) * dir;
	t2 = (node->maxz - org) * dir;

	hit.tmin4 = fmax(hit.tmin4, fmin(t1, t2));
	tmax = fmin(tmax, fmax(t1, t2));

	hit.tmini[0] = ((hit.tmini[0] & 0xFFFFFFFC) | 0b00);
	hit.tmini[1] = ((hit.tmini[1] & 0xFFFFFFFC) | 0b01);
	hit.tmini[2] = ((hit.tmini[2] & 0xFFFFFFFC) | 0b10);
	hit.tmini[3] = ((hit.tmini[3] & 0xFFFFFFFC) | 0b11);

	hit.result[0] = ((tmax.x > 0) && (hit.tmin4.x <= tmax.x) && hit.tmin4.x < ray->t) ? 1 : 0;
	hit.result[1] = ((tmax.y > 0) && (hit.tmin4.y <= tmax.y) && hit.tmin4.y < ray->t) ? 1 : 0;
	hit.result[2] = ((tmax.z > 0) && (hit.tmin4.z <= tmax.z) && hit.tmin4.z < ray->t) ? 1 : 0;
	hit.result[3] = ((tmax.w > 0) && (hit.tmin4.w <= tmax.w) && hit.tmin4.w < ray->t) ? 1 : 0;

	float tmp;
	if (hit.tmin[0] > hit.tmin[1])
	{
		tmp = hit.tmin[0];
		hit.tmin[0] = hit.tmin[1];
		hit.tmin[1] = tmp;
	}
	if (hit.tmin[2] > hit.tmin[3])
	{
		tmp = hit.tmin[2];
		hit.tmin[2] = hit.tmin[3];
		hit.tmin[3] = tmp;
	}
	if (hit.tmin[0] > hit.tmin[2])
	{
		tmp = hit.tmin[0];
		hit.tmin[0] = hit.tmin[2];
		hit.tmin[2] = tmp;
	}
	if (hit.tmin[1] > hit.tmin[3])
	{
		tmp = hit.tmin[1];
		hit.tmin[1] = hit.tmin[3];
		hit.tmin[3] = tmp;
	}
	if (hit.tmin[2] > hit.tmin[3])
	{
		tmp = hit.tmin[2];
		hit.tmin[2] = hit.tmin[3];
		hit.tmin[3] = tmp;
	}

	return hit;
}

inline void IntersectMBVHTree(global MBVHNode *nodes, global Triangle *triangles, global uint *primitiveIndices,
							  global Ray *ray)
{
	struct MBVHTraversal todo[64];
	struct MBVHHit hit;
	int stackptr = 0;

	todo[0].leftFirst = 0;
	todo[0].count = -1;

#if DEBUG_BVH
	uint steps = 0;
#endif

	while (stackptr >= 0)
	{
		struct MBVHTraversal mTodo = todo[stackptr];
		stackptr--;
#if DEBUG_BVH
		steps++;
#endif
		if (mTodo.count > -1)
		{ // leaf node
			for (int i = 0; i < mTodo.count; i++)
			{
				const int primIdx = primitiveIndices[mTodo.leftFirst + i];
				IntersectTriangle(ray, &triangles[primIdx]);
			}
			continue;
		}

		hit = IntersectMBVHNode(&nodes[mTodo.leftFirst], ray);
		if (hit.result[0] || hit.result[1] || hit.result[2] || hit.result[3])
		{
			for (int i = 3; i >= 0; i--)
			{ // reversed order, we want to check best nodes first
				const int idx = (hit.tmini[i] & 0b11);
				if (hit.result[idx] == 1)
				{
					stackptr++;
					todo[stackptr].leftFirst = nodes[mTodo.leftFirst].child[idx];
					todo[stackptr].count = nodes[mTodo.leftFirst].count[idx];
				}
			}
		}
	}

#if DEBUG_BVH
	ray->t = (float)steps;
#endif
}

inline void IntersectMBVHTreeRay(global MBVHNode *nodes, global Triangle *triangles, global uint *primitiveIndices,
								 Ray *ray)
{
	struct MBVHTraversal todo[64];
	struct MBVHHit hit;
	int stackptr = 0;

	todo[0].leftFirst = 0;
	todo[0].count = -1;

#if DEBUG_BVH
	uint steps = 0;
#endif

	while (stackptr >= 0)
	{
		struct MBVHTraversal mTodo = todo[stackptr];
		stackptr--;
#if DEBUG_BVH
		steps++;
#endif
		if (mTodo.count > -1)
		{ // leaf node
			for (int i = 0; i < mTodo.count; i++)
			{
				const int primIdx = primitiveIndices[mTodo.leftFirst + i];
				IntersectTriangleRay(ray, &triangles[primIdx]);
			}
			continue;
		}

		hit = IntersectMBVHNodeRay(&nodes[mTodo.leftFirst], ray);
		if (hit.result[0] || hit.result[1] || hit.result[2] || hit.result[3])
		{
			for (int i = 3; i >= 0; i--)
			{ // reversed order, we want to check best nodes first
				const int idx = (hit.tmini[i] & 0b11);
				if (hit.result[idx] == 1)
				{
					stackptr++;
					todo[stackptr].leftFirst = nodes[mTodo.leftFirst].child[idx];
					todo[stackptr].count = nodes[mTodo.leftFirst].count[idx];
				}
			}
		}
	}

#if DEBUG_BVH
	ray->t = (float)steps;
#endif
}

#endif