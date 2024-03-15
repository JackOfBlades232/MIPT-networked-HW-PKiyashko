#include "utils.h"
#include "common.h"
#include "raylib.h"
#include <enet/enet.h>
#include <iostream>
#include <cstring>
#include <cassert>

enum player_state_t {
    ps_waiting_for_lobby_conn,
    ps_in_lobby,
    ps_waiting_for_server_conn,
    ps_on_server
};

int main(int argc, const char **argv)
{
    int width = 800;
    int height = 600;
    InitWindow(width, height, "w6 AI MIPT");

    const int scr_width = GetMonitorWidth(0);
    const int scr_height = GetMonitorHeight(0);
    if (scr_width < width || scr_height < height) {
        width = std::min(scr_width, width);
        height = std::min(scr_height - 150, height);
        SetWindowSize(width, height);
    }

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second

    if (enet_initialize() != 0) {
        printf("Cannot init ENet");
        return 1;
    }

    ENetHost *client = enet_host_create(nullptr, 2, 2, 0, 0);
    if (!client) {
        printf("Cannot create ENet client\n");
        return 1;
    }

    ENetAddress lobby_address;
    enet_address_set_host(&lobby_address, "localhost");
    lobby_address.port = 10887;

    ENetPeer *lobby_peer = enet_host_connect(client, &lobby_address, 2, 0);
    if (!lobby_peer) {
        printf("Cannot connect to lobby");
        return 1;
    }

    ENetPeer *server_peer = nullptr;
    ENetAddress server_address;

    player_state_t state = ps_waiting_for_lobby_conn;
    std::vector<player_t> other_players;

    uint32_t time_start = enet_time_get();
    uint32_t last_pos_sent_time = time_start;
    float posx = 0.f;
    float posy = 0.f;
    while (!WindowShouldClose()) {
        const float dt = GetFrameTime();
        ENetEvent event;
        while (enet_host_service(client, &event, 10) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
                switch (state) {
                case ps_waiting_for_lobby_conn:
                    state = ps_in_lobby;
                    break;
                case ps_waiting_for_server_conn:
                    state = ps_on_server;
                    break;
                default:
                    assert(0);
                }
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                printf("Packet received '%s'\n", event.packet->data);
                switch (state) {
                case ps_in_lobby:
                {
                    assert(event.packet->dataLength == sizeof(server_address));
                    memcpy(&server_address, event.packet->data, sizeof(server_address));
                    server_peer = enet_host_connect(client, &server_address, 2, 0);
                    if (!server_peer) {
                        printf("Cannot connect to server");
                        return 1;
                    }
                    state = ps_waiting_for_server_conn;
                } break;
                case ps_on_server:
                    assert(event.packet->data[event.packet->dataLength-1] == '\0');
                    // @TODO: string parsing
                    break;
                default:
                    assert(0);
                }
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                printf("Disconnected from %s, terminating\n", event.peer == server_peer ? "server" : "lobby");
                return 1;
            default:
                break;
            };
        }

        uint32_t cur_time = enet_time_get();
        if (state == ps_on_server && cur_time - last_pos_sent_time > 500) {
            char buf[64];
            size_t buf_len = snprintf(buf, sizeof(buf), "pos:(%f,%f)", posx, posy) + 1;
            send_packet(server_peer, buf, buf_len, false, true);
            last_pos_sent_time = cur_time;
        } else if (state == ps_in_lobby && IsKeyDown(KEY_ENTER))
            send_packet(lobby_peer, (void *)"start", ARR_LEN("start"), true, true);

        bool left = IsKeyDown(KEY_LEFT);
        bool right = IsKeyDown(KEY_RIGHT);
        bool up = IsKeyDown(KEY_UP);
        bool down = IsKeyDown(KEY_DOWN);
        constexpr float spd = 10.f;
        posx += ((left ? -1.f : 0.f) + (right ? 1.f : 0.f)) * dt * spd;
        posy += ((up ? -1.f : 0.f) + (down ? 1.f : 0.f)) * dt * spd;

        BeginDrawing();
        ClearBackground(BLACK);
        DrawText(TextFormat("Current status: %s", "unknown"), 20, 20, 20, WHITE);
        DrawText(TextFormat("My position: (%d, %d)", (int)posx, (int)posy), 20, 40, 20, WHITE);
        DrawText("List of players:", 20, 60, 20, WHITE);
        EndDrawing();
    }
    return 0;
}
