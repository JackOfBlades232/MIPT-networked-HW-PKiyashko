#include "entity.hpp"
#include "protocol.hpp"

#include <enet/enet.h>

#include <cstdlib>
#include <iostream>
#include <vector>
#include <map>

static std::vector<entity_t> entities;
static std::map<uint16_t, ENetPeer *> controlled_map;

void on_join(ENetPacket *packet, ENetPeer *peer, ENetHost *host)
{
    // send all entities
    for (const entity_t &ent : entities)
        send_new_entity(peer, ent);

    // find max eid
    uint16_t maxEid = entities.empty() ? c_invalid_entity : entities[0].eid;
    for (const entity_t &e : entities)
        maxEid = std::max(maxEid, e.eid);
    uint16_t newEid = maxEid + 1;
    uint32_t color = 0xff000000 +
                     0x00440000 * (rand() % 5) +
                     0x00004400 * (rand() % 5) +
                     0x00000044 * (rand() % 5);
    float x = (rand() % 4) * 200.f;
    float y = (rand() % 4) * 200.f;
    entity_t ent = {color, x, y, newEid};
    entities.push_back(ent);

    controlled_map[newEid] = peer;

    // send info about new entity to everyone
    for (size_t i = 0; i < host->peerCount; ++i)
        send_new_entity(&host->peers[i], ent);
    // send info about controlled entity
    send_set_controlled_entity(peer, newEid);
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

int main(int argc, const char **argv)
{
    if (enet_initialize() != 0) {
        printf("Cannot init ENet");
        return 1;
    }
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
                    on_join(event.packet, event.peer, server);
                    break;
                case e_client_to_server_state:
                    on_state(event.packet);
                    break;
                };
                enet_packet_destroy(event.packet);
                break;
            default:
                break;
            };
        }
        static int t = 0;
        for (const entity_t &e : entities)
            for (size_t i = 0; i < server->peerCount; ++i) {
                ENetPeer *peer = &server->peers[i];
                if (controlled_map[e.eid] != peer)
                    send_snapshot(peer, e.eid, e.x, e.y);
            }
        //usleep(400000);
    }

    enet_host_destroy(server);

    atexit(enet_deinitialize);
    return 0;
}


