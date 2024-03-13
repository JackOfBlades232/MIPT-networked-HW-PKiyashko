#include "utils.h"
#include <enet/enet.h>
#include <iostream>

struct client_t {
    ENetPeer *peer;
    uint64_t hash;
    const char *name;
};

static const char *egyptian_names[32] =
{
 "Mohamed",
 "Youssef",
 "Ahmed",
 "Mahmoud",
 "Mustafa",
 "Yassin",
 "Taha",
 "Khaled",
 "Hamza",
 "Bilal",
 "Ibrahim",
 "Hassan",
 "Hussein",
 "Karim",
 "Tareq",
 "Abdel-Rahman",
 "Ali",
 "Omar",
 "Halim",
 "Murad",
 "Selim",
 "Abdallah"
 "Peter",
 "Pierre",
 "George",
 "John",
 "Mina",
 "Beshoi",
 "Kirollos",
 "Mark",
 "Fadi",
 "Habib"
};

uint64_t hash_client_id(size_t client_index)
{
    return (uint64_t)client_index * 0x123211123; // odd => creates a 1to1 map of Z/2^64 Z
}

const char *get_client_name(uint64_t client_hash)
{
    return egyptian_names[client_hash % ARR_LEN(egyptian_names)];
}

void remove_client(ENetPeer *peer, std::vector<client_t> &clients)
{
    for (auto it = clients.begin(); it != clients.end(); it++) {
        if (it->peer == peer) {
            clients.erase(it);
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
    address.port = 4221;

    ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

    if (!server) {
        printf("Cannot create ENet server\n");
        return 1;
    }

    uint32_t time_start = enet_time_get();
    uint32_t last_pings_sent_time = time_start;
    while (true) {
        ENetEvent event;
        while (enet_host_service(server, &event, 10) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
            { 
                printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
                // @TODO: stuff
            } break;
            case ENET_EVENT_TYPE_RECEIVE:
                printf("Packet received '%s'\n", event.packet->data);
                // @TODO: error
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                printf("Peer %x:%u disconnected\n", event.peer->address.host, event.peer->address.port);
                // @TODO: stuff
                break;
            default:
                break;
            };
        }

        uint32_t cur_time = enet_time_get();
        if (cur_time - last_pings_sent_time > 100) {
            // @TODO: ping
        }
    }

    enet_host_destroy(server);

    atexit(enet_deinitialize);
    return 0;
}

