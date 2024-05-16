#pragma once
#include <cmath>

#define F_PI 3.14159265359f

struct float2 {
    float x, y;
};

struct float3 {
    float x, y, z;
};

inline float move_to(float from, float to, float dt, float vel)
{
    float d = vel * dt;
    if (fabsf(from - to) < d)
        return to;
    
    if (to < from)
        return from - d;
    else
        return from + d;
}

inline float clamp(float in, float min, float max)
{
    return in < min ? min : in > max ? max : in;
}

inline float sign(float in)
{
    return in > 0.f ? 1.f : in < 0.f ? -1.f : 0.f;
}

template <class T>
inline T max(T a, T b)
{
    return a < b ? b : a;
}

template <class T>
inline T min(T a, T b)
{
    return a < b ? a : b;
}
