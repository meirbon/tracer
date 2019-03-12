#pragma once

#include <random>

class RandomGenerator
{
  public:
    virtual float Rand(float range = 1.0f)
    {
        return RandomUint() * 2.3283064365387e-10f * range;
    }
    virtual unsigned int RandomUint() = 0;
};