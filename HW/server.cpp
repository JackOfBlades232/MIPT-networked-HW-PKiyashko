#include "entity.hpp"
#include "protocol.hpp"

#include <enet/enet.h>

#include <cstdlib>
#include <iostream>
#include <vector>
#include <map>

static std::vector<entity_t> entities;
static std::map<ENetPeer *, uint16_t> controlled_map;

void on_join(ENetPeer *peer, ENetHost *host)
{
    // send all entities
    for (const entity_t &ent : entities)
        send_new_entity(peer, ent);

    // find max eid
    uint16_t max_eid = entities.empty() ? c_invalid_entity : entities[0].eid;
    for (const entity_t &e : entities)
        max_eid = std::max(max_eid, e.eid);
    uint16_t new_eid = max_eid + 1;
    uint32_t color = 0xff000000 +
                     0x00440000 * (rand() % 5) +
                     0x00004400 * (rand() % 5) +
                     0x00000044 * (rand() % 5);
    float x = (rand() % 4) * 200.f;
    float y = (rand() % 4) * 200.f;
    float rad = 10.f + (((float)rand() / RAND_MAX) - 0.5f) * 4.f;
    entity_t ent = {color, x, y, rad, new_eid};
    entities.push_back(ent);

    controlled_map[peer] = new_eid;

    // send info about new entity to everyone
    for (size_t i = 0; i < host->peerCount; ++i)
        send_new_entity(&host->peers[i], ent);
    // send info about controlled entity
    send_set_controlled_entity(peer, new_eid);
}

void on_state(ENetPacket *packet)
{
    uint16_t eid = c_invalid_entity;
    float x = 0.f; float y = 0.f;
    deserialize_entity_state(packet, eid, x, y);
    for (entity_t &e : entities) {
        if (e.eid == eid) {
            e.x = x;
            e.y = y;
        }
    }
}

void on_disconnect(ENetPeer *peer, ENetHost *host)
{
    uint16_t removed_eid = controlled_map[peer];
    // @SPEED(PKiyashko): if it is ever required, do a find and erase by iter
    controlled_map.erase(peer); 
    for (size_t i = 0; i < host->peerCount; ++i) {
        ENetPeer *other_peer = &host->peers[i];
        if (other_peer != peer)
            send_remove_entity(other_peer, removed_eid);
    }
}

int main(int argc, const char **argv)
{
    if (enet_initialize() != 0) {
        printf("Cannot init ENet");
        return 1;
    }
    atexit(enet_deinitialize);

    ENetAddress address;

    address.host = ENET_HOST_ANY;
    address.port = 10131;

    ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

    if (!server) {
        printf("Cannot create ENet server\n");
        return 1;
    }

    for (;;) {
        ENetEvent event;
        while (enet_host_service(server, &event, 0) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                switch (get_packet_type(event.packet)) {
                case e_client_to_server_join:
                    on_join(event.peer, server);
                    break;
                case e_client_to_server_state:
                    on_state(event.packet);
                    break;
                default:
                    break;
                };
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                printf("Peer %x:%u disconnected\n", event.peer->address.host, event.peer->address.port);
                on_disconnect(event.peer, server);
                break;
            default:
                break;
            };
        }

        for (const entity_t &e : entities)
            for (size_t i = 0; i < server->peerCount; ++i)
                send_snapshot(&server->peers[i], e.eid, e.x, e.y, e.rad);
    }

    enet_host_destroy(server);

    return 0;
}


