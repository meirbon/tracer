#pragma once

#ifdef WIN32
#include <Windows.h>
#undef max
#undef min
#endif

#include <GL/glew.h>
#include <SDL2/SDL.h>
// Do not use main from SDL2
#undef main

#include <glm/glm.hpp>

#include "Utils/RandomGenerator.h"
#include "Utils/Timer.h"

#define TO_RADIANS(degrees)                                                    \
    degrees *static_cast<float>(0.01745329251994329576923690768489)

static SDL_Window *gSDLContext = nullptr;

using uint = unsigned int;

uint Vec3ToInt(const glm::vec3 &vec);
glm::vec3 IntToVec3(const uint &u);

#ifndef PI
#define PI                                                                     \
    3.14159265358979323846264338327950288419716939937510582097494459072381640628620899862803482534211706798f
#endif
constexpr float INVPI = 1.0f / PI;

// http://www.rorydriscoll.com/2009/01/07/better-sampling/
inline glm::vec3 uniformSampleHemisphere(const float &r1, const float &r2)
{
    const float r = sqrtf(1.0f - r1 * r1);
    const float phi = 2.0f * PI * r2;
    return glm::vec3(cosf(phi) * r, r1, sinf(phi) * r);
}

inline void createCoordinateSystem(const glm::vec3 &N, glm::vec3 &Nt,
                                   glm::vec3 &Nb)
{
    if (fabs(N.x) > fabs(N.y))
    {
        Nt = glm::vec3(N.z, 0.f, -N.x) / sqrtf(N.x * N.x + N.z * N.z);
    }
    else
    {
        Nt = glm::vec3(0.f, -N.z, N.y) / sqrtf(N.y * N.y + N.z * N.z);
    }

    Nb = cross(N, Nt);
}

inline glm::vec3 localToWorld(const glm::vec3 &sample, const glm::vec3 &Nt,
                              const glm::vec3 &Nb, const glm::vec3 &normal)
{
    return glm::vec3(sample.x * Nb.x + sample.y * normal.x + sample.z * Nt.x,
                     sample.x * Nb.y + sample.y * normal.y + sample.z * Nt.y,
                     sample.x * Nb.z + sample.y * normal.z + sample.z * Nt.z);
}

inline glm::vec3 worldToLocalMicro(const glm::vec3 &vec,
                                   const glm::vec3 &rDirection, glm::vec3 &u,
                                   glm::vec3 &v, glm::vec3 &w)
{
    w = vec;
    u = glm::normalize(glm::cross(
        fabs(vec.x) > fabs(vec.y) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0),
        w));
    v = glm::cross(w, u);
    const glm::vec3 wi = -rDirection;
    return glm::normalize(
        glm::vec3(glm::dot(u, wi), glm::dot(v, wi), glm::dot(w, wi)));
}

inline glm::vec3 localToWorldMicro(const glm::vec3 &wmLocal, const glm::vec3 &u,
                                   const glm::vec3 &v, const glm::vec3 &w)
{
    return u * wmLocal.x + v * wmLocal.y + w * wmLocal.z;
}

inline glm::vec3 RandomPointOnHemisphere(const glm::vec3 &normal,
                                         RandomGenerator &rng)
{
    glm::vec3 Nt, Nb;
    createCoordinateSystem(normal, Nt, Nb);
    const float r1 = rng.Rand(1.0f);
    const float r2 = rng.Rand(1.0f);
    const glm::vec3 sample = uniformSampleHemisphere(r1, r2);
    return localToWorld(sample, Nt, Nb, normal);
}
