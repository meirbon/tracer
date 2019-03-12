#define RAND() RandomFloat(seed)

uint TauStep(int s1, int s2, int s3, uint M, uint* seed);
uint HQIRand(uint* seed);
uint SeedRandom(uint s);
uint RandomInt(uint* seed);
float RandomFloat(uint* seed);

uint TauStep(int s1, int s2, int s3, uint M, uint* seed)
{
	uint b = (((*seed << s1) ^ *seed) >> s2);
	*seed = (((*seed & M) << s3) ^ b);
	return *seed;
}

uint HQIRand(uint* seed)
{
	uint z1 = TauStep(13, 19, 12, 429496729, seed);
	uint z2 = TauStep(2, 25, 4, 4294967288, seed);
	uint z3 = TauStep(3, 11, 17, 429496280, seed);
	uint z4 = 1664525 * *seed + 1013904223;
	return z1 ^ z2 ^ z3 ^ z4;
}

uint SeedRandom(uint s)
{
	uint seed = s * 1099087573;
	seed = HQIRand(&seed);
	return seed;
}

uint RandomInt(uint* seed)
{
	// Marsaglia Xor32; see http://excamera.com/sphinx/article-xorshift.html
	// also see https://github.com/WebDrake/xorshift/blob/master/xorshift.c for higher quality variants
	*seed ^= *seed << 13;
	*seed ^= *seed >> 17;
	*seed ^= *seed << 5;
	return *seed;
}

float RandomFloat(uint* seed)
{
	return RandomInt(seed) * 2.3283064365387e-10f;
}