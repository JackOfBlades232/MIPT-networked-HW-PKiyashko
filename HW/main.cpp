// initial skeleton is a clone from https://github.com/jpcy/bgfx-minimal-example
//
#include <stdio.h>
#define WIN32_LEAN_AND_MEAN
#include "entity.hpp"
#include "protocol.hpp"
#include "math_utils.hpp"

#include "raylib.h"
#include "shared_consts.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <enet/enet.h>

#include <cstdio>
#include <cstdint>
#include <iterator>
#include <vector>
#include <deque>

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
    std::deque<entity_snapshot_t> history;
};

template <class T>
using HistoryIterator = std::deque<T>::iterator;

static ENetHost *client      = nullptr;
static ENetPeer *server_peer = nullptr;
static bool connected        = false;

static std::vector<entity_state_t> entities;

static uint16_t my_entity = c_invalid_entity;
std::deque<controls_snapshot_t> my_controls_history;

template <class T>
HistoryIterator<T> bin_search_in_history(std::deque<T> &history, int64_t ts)
{
    if (history.empty())
        return history.end();

    auto lo = history.begin();
    auto hi = history.end() - 1;

    if (ts >= hi->ts)
        return hi;
    else if (ts <= lo->ts)
        return lo;

    size_t dist;
    while ((dist = std::distance(lo, hi)) > 1) {
        auto mid = lo + dist/2;
        if (mid->ts > ts)
            hi = mid;
        else if (mid->ts < ts)
            lo = mid;
        else
            return mid;
    }

    return hi;
}

void interpolate_entity(entity_state_t &ent_state, int64_t ts)
{
    printf("inter\n");

    ent_state.sim.initialized = false;

    HistoryIterator<entity_snapshot_t> it =
        bin_search_in_history<entity_snapshot_t>(ent_state.history, ts);

    if (it == ent_state.history.begin()) {
        ent_state.ent.x = it->x;
        ent_state.ent.y = it->y;
        ent_state.ent.ori = it->ori;
    } else if (it != ent_state.history.end()) {
        HistoryIterator<entity_snapshot_t> prev = it-1;
        float coeff = (float)(it->ts - ts) / (it->ts - prev->ts);
        ent_state.ent.x = coeff*prev->x + (1.f - coeff)*it->x;
        ent_state.ent.y = coeff*prev->y + (1.f - coeff)*it->y;
        ent_state.ent.ori = coeff*prev->ori + (1.f - coeff)*it->ori;

        ent_state.history.erase(ent_state.history.begin(), prev);
    }
}

void extrapolate_entity(entity_state_t &ent_state, int64_t ts)
{
    printf("extra");

    auto init_ent_sim = [](entity_state_t &e) {
        if (e.history.empty())
            return;

        e.ent.x   = e.history.back().x;
        e.ent.y   = e.history.back().y;
        e.ent.ori = e.history.back().ori;

        if (e.ent.eid != my_entity) {
            e.ent.thr   = e.history.back().thr;
            e.ent.steer = e.history.back().steer;
        } else {
            HistoryIterator<controls_snapshot_t> it =
                bin_search_in_history<controls_snapshot_t>(my_controls_history,
                                                           e.history.back().ts);
            if (it != my_controls_history.end()) {
                e.ent.thr   = e.history.back().thr;
                e.ent.steer = e.history.back().steer;
            }
        }

        e.sim.last_ts        = e.history.back().ts;
        e.sim.start_ts       = e.sim.last_ts;
        e.sim.accumulated_dt = 0;

        e.sim.initialized = true;
    };

    if (!ent_state.sim.initialized) {
        // Set simulation initial state to last snapshot
        init_ent_sim(ent_state);
    } /* else if (ent_state.history.back().ts > ent_state.sim.start_ts) {
        // Resimulate from last snapshot
        printf(" resim");
        int64_t last_sim_ts = ent_state.sim.ts;
        init_ent_sim(ent_state);
        int64_t sim_ts;
        for (sim_ts = ent_state.history.back().ts;
             sim_ts <= last_sim_ts;
             sim_ts += c_physics_tick_ms)
        {
            // @SPEED(PKiyashko): this is quite suboptimal, make better if it's
            //                    a bottleneck
            if (ent_state.ent.eid == my_entity) {
                HistoryIterator<controls_snapshot_t> it =
                    bin_search_in_history<controls_snapshot_t>(my_controls_history, sim_ts);

                if (it != my_controls_history.end()) {
                    ent_state.ent.thr   = it->thr;
                    ent_state.ent.steer = it->steer;

                    my_controls_history.erase(my_controls_history.begin(), it);
                }
            }
            simulate_entity(ent_state.ent, (float)c_physics_tick_ms * 0.001f);
        }

        ent_state.sim.ts = sim_ts;
    } */

    putchar('\n');

    while (ent_state.sim.ts < ts) {
        int64_t step = min((int64_t)c_physics_tick_ms, ts - ent_state.sim.ts);
        simulate_entity(ent_state.ent, (float)step * 1e-3);
        ent_state.sim.ts += step;
    }
}

void calculate_entity_state(entity_state_t &ent_state, int64_t ts)
{
    if (ent_state.history.empty())
        return;

    if (ts <= ent_state.history.back().ts)
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
        if (e.ent.eid == eid)
            e.history.push_back(entity_snapshot_t{ts, x, y, ori, thr, steer});
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

    const int64_t interpolation_lag_ms = 2*c_server_tick_ms;
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

                    // Store in history
                    my_controls_history.push_back(controls_snapshot_t{ts, e.ent.thr, e.ent.steer});
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
