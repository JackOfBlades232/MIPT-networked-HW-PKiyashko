#include "entity.hpp"
#include "math_utils.hpp"

void simulate_entity(entity_t &e, float dt)
{
    bool is_braking = sign(e.thr) != 0.f && sign(e.thr) != sign(e.speed);
    float accel = is_braking ? 12.f : 3.f;
    e.speed = move_to(e.speed, clamp(e.thr, -0.3, 1.f) * 10.f, dt, accel);
    e.ori += e.steer * dt * clamp(e.speed, -2.f, 2.f) * 0.3f;
    e.x += cosf(e.ori) * e.speed * dt;
    e.y += sinf(e.ori) * e.speed * dt;
}
