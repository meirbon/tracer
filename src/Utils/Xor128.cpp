#include "Xor128.h"

#include <atomic>

static std::atomic<unsigned int> seed = 0;

Xor128::Xor128()
{
	// Prevent correlation when creating new random number generators
	unsigned int s = seed.fetch_add(1);
	for (unsigned int i = 0; i < s; i++)
		RandomUint();
}