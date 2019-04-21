#ifndef MBVH_CL
#define MBVH_CL

typedef struct
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

typedef struct
{
	int leftFirst; // Node
	int count;
} MBVHTraversal;

typedef struct
{
	union {
		float4 tmin4;
		float tmin[4];
		int tmini[4];
	};
	uint result[4];
} MBVHHit;

MBVHHit iMBVHNode(MBVHNode node, int *hit_idx, float *rayt, float3 origin, float3 inv_ray_dir);
void iMBVHTree(global MBVHNode *nodes, global int3 *indices, global float3 *vertices, global uint *primitiveIndices,
			   Ray *ray);

inline MBVHHit iMBVHNode(MBVHNode node, int *hit_idx, float *rayt, float3 origin, float3 inv_ray_dir)
{
	MBVHHit hit;

	// X axis
	float4 org = (float4)(origin.x);
	float4 dir = (float4)(inv_ray_dir.x);
	float4 t1 = (node.minx - org) * dir;
	float4 t2 = (node.maxx - org) * dir;

	hit.tmin4 = fmin(t1, t2);
	float4 tmax = fmax(t1, t2);

	// Y axis
	org = (float4)(origin.y);
	dir = (float4)(inv_ray_dir.y);
	t1 = (node.miny - org) * dir;
	t2 = (node.maxy - org) * dir;

	hit.tmin4 = fmax(hit.tmin4, fmin(t1, t2));
	tmax = fmin(tmax, fmax(t1, t2));

	// Z axis
	org = (float4)(origin.z);
	dir = (float4)(inv_ray_dir.z);
	t1 = (node.minz - org) * dir;
	t2 = (node.maxz - org) * dir;

	hit.tmin4 = fmax(hit.tmin4, fmin(t1, t2));
	tmax = fmin(tmax, fmax(t1, t2));

	hit.tmini[0] = ((hit.tmini[0] & 0xFFFFFFFC) | 0b00);
	hit.tmini[1] = ((hit.tmini[1] & 0xFFFFFFFC) | 0b01);
	hit.tmini[2] = ((hit.tmini[2] & 0xFFFFFFFC) | 0b10);
	hit.tmini[3] = ((hit.tmini[3] & 0xFFFFFFFC) | 0b11);

	hit.result[0] = ((tmax.x > 0) && (hit.tmin4.x <= tmax.x) && hit.tmin4.x < *rayt) ? 1 : 0;
	hit.result[1] = ((tmax.y > 0) && (hit.tmin4.y <= tmax.y) && hit.tmin4.y < *rayt) ? 1 : 0;
	hit.result[2] = ((tmax.z > 0) && (hit.tmin4.z <= tmax.z) && hit.tmin4.z < *rayt) ? 1 : 0;
	hit.result[3] = ((tmax.w > 0) && (hit.tmin4.w <= tmax.w) && hit.tmin4.w < *rayt) ? 1 : 0;

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

inline void iMBVHTree(global MBVHNode *nodes, global int3 *indices, global float3 *vertices,
					  global uint *primitiveIndices, Ray *ray)
{
	MBVHTraversal todo[64];
	MBVHHit hit;
	int stackptr = 0;

	todo[0].leftFirst = 0;
	todo[0].count = -1;

	float3 inv_dir = 1.0f / ray->direction;

	while (stackptr >= 0)
	{
		MBVHTraversal mTodo = todo[stackptr];
		stackptr--;

		if (mTodo.count > -1)
		{ // leaf node
			for (int i = 0; i < mTodo.count; i++)
			{
				const int primIdx = primitiveIndices[mTodo.leftFirst + i];
				int3 idx = indices[primIdx];
				float3 p0 = vertices[idx.x];
				float3 p1 = vertices[idx.y];
				float3 p2 = vertices[idx.z];
				if (t_intersect(ray->origin, ray->direction, &ray->t, p0, p1, p2, 0.00001f))
					ray->hit_idx = primIdx;
			}
			continue;
		}

		hit = iMBVHNode(nodes[mTodo.leftFirst], &ray->hit_idx, &ray->t, ray->origin, inv_dir);
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
}
#endif