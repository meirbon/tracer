# Tracer

A ray/path tracing application.

## Features

- Implements a cache-aligned BVH & 4-way MBVH with the following types: SAH, Binned SAH & Central split
- Ray & path tracer for CPU
- OpenCL path tracer on GPU
- Sphere, plane, torus & triangles on CPU & triangles on GPU
- Variance reduction: Next Event Estimation & Multiple Importance Sampling
- Lambert Diffuse BRDF & Microfacet BRDF (GGX)

## Building

Using CMake this project can built on Mac, Windows & Linux. All platforms have been successfully tested.

### Requirements
- GLEW
- SDL2
- OpenCL
- GLM
- FreeImage

Libraries have been included for Windows and CMake is configured to automatically link to the libs included in this
repository. For Linux/Mac you should install GLEW, SDL2, FreeImage and (only on Linux) OpenCL libraries.

## Controls

Switching between the modes can be done using the following buttons:
- 1 for Reference (CPU + GPU)
- 2 for Next Event Estimation (NEE) (CPU)
- 3 for Importance Sampling (IS) (CPU)
- 4 for NEE + IS (CPU)
- 5 for NEE + Multiple IS (CPU + GPU)
- 6 for Reference + Microfacets (CPU + GPU)

The camera can be controlled using the mouse & WASD. View direction can also be changed using the arrow keys.
The camera can be locked/unlocked by pressing L.