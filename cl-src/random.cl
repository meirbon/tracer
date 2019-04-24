#ifndef RANDOM_CL
#define RANDOM_CL

uint RandomInt(uint *seed);
uint getSeed(uint threadID, uint frameID);
float RandomFloat(uint *seed);
uint RandomIntMax(uint *seed, int max);

uint RandomInt(uint *seed)
{
	// Marsaglia Xor32; see http://excamera.com/sphinx/article-xorshift.html
	// also see https://github.com/WebDrake/xorshift/blob/master/xorshift.c for higher quality variants
	*seed ^= *seed << 13;
	*seed ^= *seed >> 17;
	*seed ^= *seed << 5;
	return *seed;
}

inline uint getSeed(uint threadID, uint frameID) { return (frameID * threadID * 147565741) * 720898027; }

inline float RandomFloat(uint *seed) { return RandomInt(seed) * 2.3283064365387e-10f; }

inline uint RandomIntMax(uint *seed, int max) { return int(RandomFloat(seed) * (max + 0.99999f)); }

#endif