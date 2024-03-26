#pragma once
#include <cstdint>

constexpr uint16_t c_invalid_entity = -1;

struct entity_t {
    uint32_t color = 0xff00ffff;
    float x = 0.f;
    float y = 0.f;
    uint16_t eid = c_invalid_entity;
};

