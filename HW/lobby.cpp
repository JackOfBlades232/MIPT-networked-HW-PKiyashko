#include "utils.h"
#include <enet/enet.h>
#include <cstring>
#include <iostream>
#include <vector>

int main(int argc, const char **argv)
{
    if (enet_initialize() != 0) {
        printf("Cannot init ENet");
        return 1;
    }
    ENetAddress address;

    address.host = ENET_HOST_ANY;
    address.port = 10887;

    ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

    if (!server) {
        printf("Cannot create ENet server\n");
        return 1;
    }

    ENetAddress game_serv_address;
    enet_address_set_host(&game_serv_address, "localhost");
    game_serv_address.port = 4221;

    bool game_started = false;

    while (true) {
        ENetEvent event;
        while (enet_host_service(server, &event, 10) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
            {
                printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
                if (game_started)
                    send_packet(event.peer, &game_serv_address, sizeof(game_serv_address), true, true); 
            } break;
            case ENET_EVENT_TYPE_RECEIVE:
            {
                printf("Packet received '%s'\n", event.packet->data);
                if (packet_is_string(event.packet) && PACKET_IS_STAT_STRING(event.packet, "start")) {
                    if (!game_started) {
                        game_started = true;
                        for (size_t i = 0; i < server->connectedPeers; i++)
                            send_packet(&server->peers[i], &game_serv_address, sizeof(game_serv_address), true, true); 
                    }
                }
                enet_packet_destroy(event.packet);
            } break;
            case ENET_EVENT_TYPE_DISCONNECT:
                printf("Peer %x:%u disconnected\n", event.peer->address.host, event.peer->address.port);
                break;
            default:
                break;
            };
        }
    }

    enet_host_destroy(server);

    atexit(enet_deinitialize);
    return 0;
}

