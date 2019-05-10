#include "Shared.h"

#define RAY_MASK_NO_HIT -1
#define RAY_MASK_REGENERATE -2

// clang-format off
#include "random.cl"
#include "triangle.cl"
#include "material.cl"
#include "ray.cl"
#include "mbvh.cl"
#include "microfacet.cl"
// clang-format onZ