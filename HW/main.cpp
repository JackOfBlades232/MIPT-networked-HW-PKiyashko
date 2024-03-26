#define WIN32_LEAN_AND_MEAN
#include "entity.h"
#include "protocol.h"

#include "raylib.h"
#include <enet/enet.h>

#include <functional>
#include <algorithm> // min/max
#include <vector>

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
    float x = 0.f; float y = 0.f;
    deserialize_snapshot(packet, eid, x, y);
    // TODO: Direct adressing, of course!
    for (entity_t &e : entities) {
        if (e.eid == eid) {
            e.x = x;
            e.y = y;
        }
    }
}

int main(int argc, const char **argv)
{
    if (enet_initialize() != 0) {
        printf("Cannot init ENet");
        return 1;
    }

    ENetHost *client = enet_host_create(nullptr, 1, 2, 0, 0);
    if (!client) {
        printf("Cannot create ENet client\n");
        return 1;
    }

    ENetAddress address;
    enet_address_set_host(&address, "127.0.0.1");
    address.port = 10131;

    ENetPeer *server_peer = enet_host_connect(client, &address, 2, 0);
    if (!server_peer) {
        printf("Cannot connect to server");
        return 1;
    }

    int width = 700;
    int height = 700;
    InitWindow(width, height, "w4 networked MIPT");

    const int scr_width = GetMonitorWidth(0);
    const int scr_height = GetMonitorHeight(0);
    if (scr_width < width || scr_height < height) {
        width = std::min(scr_width, width);
        height = std::min(scr_height - 150, height);
        SetWindowSize(width, height);
    }

    Camera2D camera = {{0, 0}, {0, 0}, 0.f, 1.f};
    camera.target = Vector2{0.f, 0.f};
    camera.offset = Vector2{width * 0.5f, height * 0.5f};
    camera.rotation = 0.f;
    camera.zoom = 1.f;

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second

    bool connected = false;
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        ENetEvent event;
        while (enet_host_service(client, &event, 0) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
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
                };
                break;
            default:
                break;
            };
        }
        if (my_entity != c_invalid_entity) {
            bool left = IsKeyDown(KEY_LEFT);
            bool right = IsKeyDown(KEY_RIGHT);
            bool up = IsKeyDown(KEY_UP);
            bool down = IsKeyDown(KEY_DOWN);
            // TODO: Direct adressing, of course!
            for (entity_t &e : entities) {
                if (e.eid == my_entity) {
                    // Update
                    e.x += ((left ? -dt : 0.f) + (right ? +dt : 0.f)) * 100.f;
                    e.y += ((up ? -dt : 0.f) + (down ? +dt : 0.f)) * 100.f;

                    // Send
                    send_entity_state(server_peer, my_entity, e.x, e.y);
                }
            }
        }

        BeginDrawing();
        {
            ClearBackground(GRAY);
            BeginMode2D(camera);
            {
                for (const entity_t &e : entities) {
                    const Rectangle rect = {e.x, e.y, 10.f, 10.f};
                    DrawRectangleRec(rect, GetColor(e.color));
                }
            }
            EndMode2D();
        }
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
