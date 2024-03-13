#include "utils.h"
#include <enet/enet.h>
#include <iostream>
#include <vector>
#include <cstring>

static void remove_peer(std::vector<ENetPeer *> &peers, ENetPeer *peer)
{
    for (auto it = peers.begin(); it != peers.end(); it++) {
        if (*it == peer) {
            peers.erase(it);
            break;
        }
    }
}

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
    address.port = 4221;

    bool game_started = false;
    std::vector<ENetPeer *> peers;

    while (true) {
        ENetEvent event;
        while (enet_host_service(server, &event, 10) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
                peers.push_back(event.peer);
                break;
            case ENET_EVENT_TYPE_RECEIVE:
            {
                printf("Packet received '%s'\n", event.packet->data);
                if (ARR_LEN("start") != event.packet->dataLength || strcmp("start", (const char *)event.packet->data) != 0) {
                    if (game_started)
                        send_packet(event.peer, &game_serv_address, sizeof(game_serv_address), true, true); 
                    else {
                        for (ENetPeer *peer : peers)
                            send_packet(peer, &game_serv_address, sizeof(game_serv_address), true, true); 
                    }
                }
                else {
                    send_packet(event.peer, (void *)"You f*@#ed up", sizeof("You f*@#ed up"), true, true); 
                    enet_peer_disconnect(event.peer, 0);
                    remove_peer(peers, event.peer);
                }
                enet_packet_destroy(event.packet);
            } break;
            case ENET_EVENT_TYPE_DISCONNECT:
                printf("Peer %x:%u disconnected\n", event.peer->address.host, event.peer->address.port);
                remove_peer(peers, event.peer);
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

