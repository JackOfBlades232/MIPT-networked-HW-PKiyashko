#pragma once
#include <cstdint>

constexpr uint16_t c_invalid_entity = -1;

// @NOTE(PKiyashko): we trust the padding here, cause there is no good cross-
//                   compiler way of disabling padding. This is not too good.
struct entity_t {
    uint32_t color = 0xff00ffff;
    float x        = 0.f;
    float y        = 0.f;
    float rad      = 0.f;

    uint16_t eid = c_invalid_entity;
};

