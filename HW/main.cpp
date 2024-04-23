// initial skeleton is a clone from https://github.com/jpcy/bgfx-minimal-example
//
#include "entity.hpp"
#include "protocol.hpp"

#include "raylib.h"
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <enet/enet.h>

#include <cmath>
#include <cstdio>
#include <vector>

static ENetHost *client      = nullptr;
static ENetPeer *server_peer = nullptr;
static bool connected        = false;

static std::vector<entity_t> entities;
static uint16_t my_entity = c_invalid_entity;

void on_new_entity_packet(ENetPacket *packet)
{
    entity_t new_entity;
    deserialize_new_entity(packet, new_entity);
    // TODO: Direct adressing, of course!
    for (const entity_t &e : entities)
        if (e.eid == new_entity.eid)
            return; // don't need to do anything, we already have entity
    entities.push_back(new_entity);
}

void on_set_controlled_entity(ENetPacket *packet)
{
    deserialize_set_controlled_entity(packet, my_entity);
}

void on_snapshot(ENetPacket *packet)
{
    uint16_t eid = c_invalid_entity;
    float x      = 0.f;
    float y      = 0.f;
    float ori    = 0.f;
    deserialize_snapshot(packet, eid, x, y, ori);
    // TODO: Direct adressing, of course!
    for (entity_t &e : entities)
        if (e.eid == eid) {
            e.x   = x;
            e.y   = y;
            e.ori = ori;
        }
}

void on_remove_entity(ENetPacket *packet)
{
    uint16_t eid = c_invalid_entity;
    deserialize_remove_entity(packet, eid);
    for (auto it = entities.begin(); it != entities.end(); ++it)
        if (it->eid == eid) {
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

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
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
                default:
                    assert(0);
                };
                break;
            default:
                break;
            };
        }

        if (my_entity != c_invalid_entity) {
            bool left  = IsKeyDown(KEY_LEFT);
            bool right = IsKeyDown(KEY_RIGHT);
            bool up    = IsKeyDown(KEY_UP);
            bool down  = IsKeyDown(KEY_DOWN);
            // TODO: Direct adressing, of course!
            for (entity_t &e : entities)
                if (e.eid == my_entity) {
                    // Update
                    float thr   = (up ? 1.f : 0.f) + (down ? -1.f : 0.f);
                    float steer = (left ? -1.f : 0.f) + (right ? 1.f : 0.f);

                    // Send
                    send_entity_input(server_peer, my_entity, thr, steer);
                }
        }

        BeginDrawing();
        ClearBackground(GRAY);
        BeginMode2D(camera);
        for (const entity_t &e : entities) {
            const Rectangle rect = {e.x, e.y, 3.f, 1.f};
            DrawRectanglePro(
                rect, {0.f, 0.5f}, e.ori * 180.f / PI, GetColor(e.color));
        }

        EndMode2D();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
