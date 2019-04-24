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
		float4 tmin;
		int4 tmini;
		int tmini4[4];
	};
	union {
		int4 result;
		int result4[4];
	};
} MBVHHit;

MBVHHit iMBVHNode(MBVHNode node, int *hit_idx, float *rayt, float3 origin, float3 inv_ray_dir);
void iMBVHTree(global MBVHNode *nodes, global int3 *indices, global float3 *vertices, global uint *primIdxs,
			   float *rayt, int *hit_idx, float3 origin, float3 dir);

inline MBVHHit iMBVHNode(MBVHNode node, int *hit_idx, float *rayt, float3 origin, float3 inv_ray_dir)
{
	MBVHHit hit;

	// X axis
	float4 org = (float4)(origin.x);
	float4 dir = (float4)(inv_ray_dir.x);
	float4 t1 = (node.minx - org) * dir;
	float4 t2 = (node.maxx - org) * dir;

	hit.tmin = fmin(t1, t2);
	float4 tmax = fmax(t1, t2);

	// Y axis
	org = (float4)(origin.y);
	dir = (float4)(inv_ray_dir.y);
	t1 = (node.miny - org) * dir;
	t2 = (node.maxy - org) * dir;

	hit.tmin = fmax(hit.tmin, fmin(t1, t2));
	tmax = fmin(tmax, fmax(t1, t2));

	// Z axis
	org = (float4)(origin.z);
	dir = (float4)(inv_ray_dir.z);
	t1 = (node.minz - org) * dir;
	t2 = (node.maxz - org) * dir;

	hit.tmin = fmax(hit.tmin, fmin(t1, t2));
	tmax = fmin(tmax, fmax(t1, t2));

	hit.tmini = (hit.tmini & (int4)(0xFFFFFFFC)) | ((int4)(0b00, 0b01, 0b10, 0b11));

	//	hit.result = (tmax > (float4)(0)) && (hit.tmin <= tmax) && (hit.tmin < (float4)(*rayt));

	hit.result = tmax > ((float4)(0)) & hit.tmin <= tmax;
	hit.result4[0] &= (hit.tmin.x < *rayt);
	hit.result4[1] &= (hit.tmin.y < *rayt);
	hit.result4[2] &= (hit.tmin.z < *rayt);
	hit.result4[3] &= (hit.tmin.w < *rayt);

	if (hit.tmin[0] > hit.tmin[1])
	{
		hit.tmini4[0] = hit.tmini4[0] ^ hit.tmini4[1];
		hit.tmini4[1] = hit.tmini4[0] ^ hit.tmini4[1];
		hit.tmini4[0] = hit.tmini4[0] ^ hit.tmini4[1];
	}
	if (hit.tmin[2] > hit.tmin[3])
	{
		hit.tmini4[2] = hit.tmini4[2] ^ hit.tmini4[3];
		hit.tmini4[3] = hit.tmini4[2] ^ hit.tmini4[3];
		hit.tmini4[2] = hit.tmini4[2] ^ hit.tmini4[3];
	}
	if (hit.tmin[0] > hit.tmin[2])
	{
		hit.tmini4[0] = hit.tmini4[0] ^ hit.tmini4[2];
		hit.tmini4[2] = hit.tmini4[0] ^ hit.tmini4[2];
		hit.tmini4[0] = hit.tmini4[0] ^ hit.tmini4[2];
	}
	if (hit.tmin[1] > hit.tmin[3])
	{
		hit.tmini4[1] = hit.tmini4[1] ^ hit.tmini4[3];
		hit.tmini4[3] = hit.tmini4[1] ^ hit.tmini4[3];
		hit.tmini4[1] = hit.tmini4[1] ^ hit.tmini4[3];
	}
	if (hit.tmin[2] > hit.tmin[3])
	{
		hit.tmini4[2] = hit.tmini4[2] ^ hit.tmini4[3];
		hit.tmini4[3] = hit.tmini4[2] ^ hit.tmini4[3];
		hit.tmini4[2] = hit.tmini4[2] ^ hit.tmini4[3];
	}

	return hit;
}

void iMBVHTree(global MBVHNode *nodes, global int3 *indices, global float3 *vertices, global uint *primIdxs,
			   float *rayt, int *hit_idx, float3 origin, float3 dir)
{
	MBVHTraversal todo[64];
	MBVHHit hit;
	int stackptr = 0;

	todo[0].leftFirst = 0;
	todo[0].count = -1;

	float3 inv_dir = 1.0f / dir;

	while (stackptr >= 0)
	{
		MBVHTraversal mTodo = todo[stackptr];
		stackptr--;

		if (mTodo.count > -1)
		{ // leaf node
			for (int i = 0; i < mTodo.count; i++)
			{
				const int primIdx = primIdxs[mTodo.leftFirst + i];
				int3 idx = indices[primIdx];
				float3 p0 = vertices[idx.x];
				float3 p1 = vertices[idx.y];
				float3 p2 = vertices[idx.z];
				if (t_intersect(origin, dir, rayt, p0, p1, p2, EPSILON))
					*hit_idx = primIdx;
			}
		}
		else
		{
			hit = iMBVHNode(nodes[mTodo.leftFirst], hit_idx, rayt, origin, inv_dir);
			const int hasHit = hit.result.x | hit.result.y | hit.result.z | hit.result.w;
			if (hasHit)
			{
				for (int i = 3; i >= 0; i--)
				{ // reversed order, we want to check best nodes first
					const int idx = (hit.tmini4[i] & 0b11);
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
}
#endif