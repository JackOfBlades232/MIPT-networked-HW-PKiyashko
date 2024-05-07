#include "entity.hpp"
#include "protocol.hpp"
#include "shared_consts.hpp"

#include <cassert>
#include <cstdint>
#include <enet/enet.h>

#include <cstdio>
#include <cstdlib>
#include <map>
#include <vector>
#include <thread>
#include <chrono>

static std::vector<entity_t> entities;
static std::map<uint16_t, ENetPeer *> controlled_map;

void on_join(ENetPacket *packet, ENetPeer *peer, ENetHost *host)
{
    // send all entities
    for (const entity_t &ent : entities)
        send_new_entity(peer, ent);

    // find max eid
    uint16_t max_eid = entities.empty() ? c_invalid_entity : entities[0].eid;
    for (const entity_t &e : entities)
        max_eid = std::max(max_eid, e.eid);
    uint16_t new_eid = max_eid + 1;
    uint32_t color   = 0xff000000 + 0x00440000 * (rand() % 5) +
                       0x00004400 * (rand() % 5) + 0x00000044 * (rand() % 5);
    float x = (rand() % 4) * 5.f;
    float y = (rand() % 4) * 5.f;

    entity_t ent = {
        color, x, y, 0.f, 
        ((float)rand() / RAND_MAX) * 3.141592654f, 
        0.f, 0.f, 
        new_eid
    };
    entities.push_back(ent);

    controlled_map[new_eid] = peer;

    // send info about new entity to everyone
    for (size_t i = 0; i < host->peerCount; ++i)
        send_new_entity(&host->peers[i], ent);
    // send info about controlled entity
    send_set_controlled_entity(peer, new_eid);
}

void on_disconnect(ENetPeer *peer, ENetHost *host)
{
    uint16_t eid = c_invalid_entity;
    for (auto it = controlled_map.begin(); it != controlled_map.end(); ++it) {
        if (it->second == peer) {
            eid = it->first;
            controlled_map.erase(it);
            break;
        }
    }

    if (eid == c_invalid_entity) // Was already removed
        return;

    for (auto it = entities.begin(); it != entities.end(); ++it) {
        if (it->eid == eid) {
            entities.erase(it);
            break;
        }
    }

    for (size_t i = 0; i < host->peerCount; ++i) {
        ENetPeer *other_peer = &host->peers[i];
        if (other_peer->state == ENET_PEER_STATE_CONNECTED &&
            other_peer != peer) 
        {
            send_remove_entity(other_peer, eid);
        }
    }
}

void on_input(ENetPacket *packet)
{
    uint16_t eid = c_invalid_entity;
    float thr    = 0.f;
    float steer  = 0.f;
    deserialize_entity_input(packet, eid, thr, steer);
    for (entity_t &e : entities)
        if (e.eid == eid) {
            e.thr   = thr;
            e.steer = steer;
        }
}

int main(int argc, const char **argv)
{
    srand(0xFEFECC);

    if (enet_initialize() != 0) {
        printf("Cannot init ENet");
        return 1;
    }
    atexit(&enet_deinitialize);

    ENetAddress address;

    address.host = ENET_HOST_ANY;
    address.port = 10131;

    ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

    if (!server) {
        printf("Cannot create ENet server\n");
        return 1;
    }

    int64_t last_time      = enet_time_get();
    int64_t accumulated_dt = 0;
    while (true) {
        int64_t pre_event_loop_time = enet_time_get();

        ENetEvent event;
        while (enet_host_service(server, &event, 0) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                printf("Connection with %x:%u established\n",
                       event.peer->address.host,
                       event.peer->address.port);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                on_disconnect(event.peer, server);
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                switch (get_packet_type(event.packet)) {
                case e_client_to_server_join:
                    on_join(event.packet, event.peer, server);
                    break;
                case e_client_to_server_disconnect:
                    on_disconnect(event.peer, server);
                    break;
                case e_client_to_server_input:
                    on_input(event.packet);
                    break;
                default:
                    assert(0);
                };
                enet_packet_destroy(event.packet);
                break;
            default:
                break;
            };
        }

        int64_t cur_time = enet_time_get();
        accumulated_dt += cur_time - last_time;
        last_time = cur_time;

        int nticks = accumulated_dt / c_physics_tick_ms;
        accumulated_dt -= c_physics_tick_ms * nticks;
        for (entity_t &e : entities) {
            // simulate
            for (int i = 0; i < nticks; ++i)
                simulate_entity(e, PHYSICS_TICK_SEC);

            // send
            for (size_t i = 0; i < server->peerCount; ++i) {
                ENetPeer *peer = &server->peers[i];
                if (peer->state == ENET_PEER_STATE_CONNECTED)
                {
                    // skip this here in this implementation
                    // if (controlledMap[e.eid] != peer)
                    send_snapshot(
                        peer, cur_time, e.eid, e.x, e.y, e.ori, e.thr, e.steer);
                }
            }
        }
        int64_t frame_time = enet_time_get() - pre_event_loop_time;
        if (frame_time < c_server_tick_ms)
            std::this_thread::sleep_for(std::chrono::milliseconds(c_server_tick_ms - frame_time));
    }

    enet_host_destroy(server);
    return 0;
}
