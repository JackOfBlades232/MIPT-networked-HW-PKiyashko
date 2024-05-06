#pragma once

#include <cstdint>

enum : uint32_t {
    c_physics_tick_ms = 10,
    c_server_tick_ms = 100
};

#define PHYSICS_TICK_SEC ((float)c_physics_tick_ms * 1e-3)
