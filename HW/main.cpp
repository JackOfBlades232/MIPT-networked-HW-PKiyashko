// initial skeleton is a clone from https://github.com/jpcy/bgfx-minimal-example
//
#include <stdio.h>
#define WIN32_LEAN_AND_MEAN
#include "entity.hpp"
#include "protocol.hpp"
#include "history.hpp"
#include "shared_consts.hpp"

#include "raylib.h"
#include <enet/enet.h>

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <vector>

struct controls_snapshot_t {
    int64_t ts;
    float thr;
    float steer;
};

struct entity_snapshot_t {
    int64_t ts;
    float x;
    float y;
    float ori;
    float thr;
    float steer;
};

struct entity_state_t {
    entity_t ent;
    struct {
        int64_t accumulated_dt, last_ts, start_ts;
        bool initialized = false;
    } sim;
    History<entity_snapshot_t> history;
};

static ENetHost *client      = nullptr;
static ENetPeer *server_peer = nullptr;
static bool connected        = false;

static std::vector<entity_state_t> entities;

static uint16_t my_entity = c_invalid_entity;
History<controls_snapshot_t> my_controls_history;

void interpolate_entity(entity_state_t &ent_state, int64_t ts)
{
    printf("inter\n");

    ent_state.sim.initialized = false;

    auto [prev, next] = ent_state.history.FetchTwoClosest(ts);
    assert(next); // must be <= last ts to interpolate correclty

    if (!prev || prev->ts == next->ts) {
        ent_state.ent.x   = next->x;
        ent_state.ent.y   = next->y;
        ent_state.ent.ori = next->ori;
    } else {
        float coeff = (float)(next->ts - ts) / (next->ts - prev->ts);
        ent_state.ent.x = coeff*prev->x + (1.f - coeff)*next->x;
        ent_state.ent.y = coeff*prev->y + (1.f - coeff)*next->y;
        ent_state.ent.ori = coeff*prev->ori + (1.f - coeff)*next->ori;
    }
}

void extrapolate_entity(entity_state_t &ent_state, int64_t ts)
{
    printf("extra");

    auto init_ent_sim = [](entity_state_t &e) {
        if (e.history.Empty())
            return;

        const entity_snapshot_t *last = e.history.FetchBack();

        e.ent.x   = last->x;
        e.ent.y   = last->y;
        e.ent.ori = last->ori;

        if (e.ent.eid != my_entity) {
            e.ent.thr   = last->thr;
            e.ent.steer = last->steer;
        }

        e.sim.last_ts        = last->ts;
        e.sim.start_ts       = e.sim.last_ts;
        e.sim.accumulated_dt = 0;

        e.sim.initialized = true;
    };

    if (!ent_state.sim.initialized) {
        // Set simulation initial state to last snapshot
        init_ent_sim(ent_state);
    } else if (ent_state.history.Back()->ts > ent_state.sim.start_ts) {
        // Resimulate from last snapshot
        printf(" resim");
        init_ent_sim(ent_state);
    }

    putchar('\n');

    ent_state.sim.accumulated_dt += ts - ent_state.sim.last_ts;

    int nticks = ent_state.sim.accumulated_dt / c_physics_tick_ms;
    ent_state.sim.accumulated_dt -= nticks * c_physics_tick_ms;
    for (int i = 0; i < nticks; ++i) {
        if (ent_state.ent.eid == my_entity) {
            const controls_snapshot_t *controls =
                my_controls_history.FetchLastBefore(ent_state.sim.last_ts +
                                                    i * c_physics_tick_ms);

            if (controls) {
                ent_state.ent.thr   = controls->thr;
                ent_state.ent.steer = controls->steer;
            }
        }

        simulate_entity(ent_state.ent, PHYSICS_TICK_SEC);
    }

    ent_state.sim.last_ts = ts;
}

void calculate_entity_state(entity_state_t &ent_state, int64_t ts)
{
    if (ent_state.history.Empty())
        return;

    if (ts <= ent_state.history.Back()->ts)
        interpolate_entity(ent_state, ts);
    else
        extrapolate_entity(ent_state, ts);
}

void on_new_entity_packet(ENetPacket *packet)
{
    entity_t new_entity;
    deserialize_new_entity(packet, new_entity);
    // TODO: Direct adressing, of course!
    for (const entity_state_t &e : entities)
        if (e.ent.eid == new_entity.eid)
            return; // don't need to do anything, we already have entity
    entities.push_back(entity_state_t{new_entity});
}

void on_set_controlled_entity(ENetPacket *packet)
{
    deserialize_set_controlled_entity(packet, my_entity);
}

void on_snapshot(ENetPacket *packet)
{
    int64_t ts   = 0;
    uint16_t eid = c_invalid_entity;
    float x      = 0.f;
    float y      = 0.f;
    float ori    = 0.f;
    float thr    = 0.f;
    float steer  = 0.f;
    deserialize_snapshot(packet, ts, eid, x, y, ori, thr, steer);

    // TODO: Direct adressing, of course!
    for (entity_state_t &e : entities)
        if (e.ent.eid == eid) {
            e.history.Push(entity_snapshot_t{ts, x, y, ori, thr, steer});
            break;
        }
}

void on_remove_entity(ENetPacket *packet)
{
    uint16_t eid = c_invalid_entity;
    deserialize_remove_entity(packet, eid);
    for (auto it = entities.begin(); it != entities.end(); ++it)
        if (it->ent.eid == eid) {
            entities.erase(it);
            break;
        }
}

void on_quit()
{
    if (connected) {
        send_disconnect(server_peer);
        enet_host_flush(client);
    }
}

int main(int argc, const char **argv)
{
    if (enet_initialize() != 0) {
        printf("Cannot init ENet");
        return 1;
    }

    client = enet_host_create(nullptr, 1, 2, 0, 0);
    if (!client) {
        printf("Cannot create ENet client\n");
        return 1;
    }

    ENetAddress address;
    enet_address_set_host(&address, "localhost");
    address.port = 10131;

    server_peer = enet_host_connect(client, &address, 2, 0);
    if (!server_peer) {
        printf("Cannot connect to server");
        return 1;
    }

    atexit(&on_quit);

    int width  = 600;
    int height = 600;

    InitWindow(width, height, "HW3 networked MIPT");

    const int scr_width  = GetMonitorWidth(0);
    const int scr_height = GetMonitorHeight(0);
    if (scr_width < width || scr_height < height) {
        width  = std::min(scr_width, width);
        height = std::min(scr_height - 150, height);
        SetWindowSize(width, height);
    }

    Camera2D camera = { {0, 0}, {0, 0}, 0.f, 1.f };
    camera.target   = Vector2{0.f, 0.f};
    camera.offset   = Vector2{width * 0.5f, height * 0.5f};
    camera.rotation = 0.f;
    camera.zoom     = 10.f;

    SetTargetFPS(60); // Set our game to run at 60 frames-per-second

    const int64_t interpolation_lag_ms = c_server_tick_ms*2;
    while (!WindowShouldClose()) {
        ENetEvent event;
        while (enet_host_service(client, &event, 0) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                printf("Connection with %x:%u established\n",
                       event.peer->address.host,
                       event.peer->address.port);
                send_join(server_peer);
                connected = true;
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                switch (get_packet_type(event.packet)) {
                case e_server_to_client_new_entity:
                    on_new_entity_packet(event.packet);
                    break;
                case e_server_to_client_set_controlled_entity:
                    on_set_controlled_entity(event.packet);
                    break;
                case e_server_to_client_snapshot:
                    on_snapshot(event.packet);
                    break;
                case e_server_to_client_remove_entity:
                    on_remove_entity(event.packet);
                    break;
                default: assert(0);
                };
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                goto done;
            default:
                break;
            };
        }

        int64_t cur_ts = enet_time_get();
        int64_t ts = cur_ts - interpolation_lag_ms;

        if (my_entity != c_invalid_entity) {
            bool left  = IsKeyDown(KEY_LEFT);
            bool right = IsKeyDown(KEY_RIGHT);
            bool up    = IsKeyDown(KEY_UP);
            bool down  = IsKeyDown(KEY_DOWN);
            // TODO: Direct adressing, of course!
            for (entity_state_t &e : entities) {
                // Send controls
                if (e.ent.eid == my_entity) {
                    // Update
                    e.ent.thr   = (up ? 1.f : 0.f) + (down ? -1.f : 0.f);
                    e.ent.steer = (left ? -1.f : 0.f) + (right ? 1.f : 0.f);

                    // Send
                    send_entity_input(server_peer, my_entity, e.ent.thr, e.ent.steer);

                    // Store in history (not offsetting by interp lag, it would be too early)
                    my_controls_history.Push(controls_snapshot_t{cur_ts, e.ent.thr, e.ent.steer});
                }

              // Interpolate/Extrapolate/Resimulate
              calculate_entity_state(e, ts);
            }
        }

        BeginDrawing();
        ClearBackground(GRAY);
        BeginMode2D(camera);
        for (const entity_state_t &e : entities) {
            const Rectangle rect = {e.ent.x, e.ent.y, 3.f, 1.f};
            DrawRectanglePro(
                rect, {0.f, 0.5f}, e.ent.ori * 180.f / PI, GetColor(e.ent.color));
        }

        EndMode2D();
        EndDrawing();
    }

done:
    CloseWindow();
    return 0;
}
