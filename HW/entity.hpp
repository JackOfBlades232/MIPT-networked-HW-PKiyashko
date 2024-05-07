#pragma once
#include <cstdint>

constexpr uint16_t c_invalid_entity = -1;

// @NOTE(PKiyashko): we trust the padding here, cause there is no good cross-
//                   compiler way of disabling padding. This is not too good.
struct entity_t {
    uint32_t color = 0xff00ffff;

    float x     = 0.f;
    float y     = 0.f;
    float speed = 0.f;
    float ori   = 0.f;

    float thr   = 0.f;
    float steer = 0.f;

    uint16_t eid = c_invalid_entity;
};

struct controls_snapshot_t {
    int64_t ts;
    float thr;
    float steer;
};

void simulate_entity(entity_t &e, float dt);
