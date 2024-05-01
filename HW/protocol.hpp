#pragma once
#include "entity.hpp"

#include <enet/enet.h>

#include <cstdint>
#include <stdint.h>

enum message_type_t : uint8_t {
    e_client_to_server_join = 0,
    e_client_to_server_disconnect,
    e_server_to_client_new_entity,
    e_server_to_client_set_controlled_entity,
    e_server_to_client_remove_entity,
    e_client_to_server_input,
    e_server_to_client_clock_sync,
    e_server_to_client_snapshot
};

void send_join(ENetPeer *peer);
void send_disconnect(ENetPeer *peer);
void send_new_entity(ENetPeer *peer, const entity_t &ent);
void send_set_controlled_entity(ENetPeer *peer, uint16_t eid);
void send_remove_entity(ENetPeer *peer, uint16_t eid);
void send_entity_input(ENetPeer *peer, uint16_t eid, float thr, float steer);
void send_sync_clock(ENetPeer *peer, int64_t time);
void send_snapshot(ENetPeer *peer, int64_t time, uint16_t eid, float x,
                   float y, float ori, float thr, float steer);

message_type_t get_packet_type(ENetPacket *packet);

void deserialize_new_entity(ENetPacket *packet, entity_t &ent);
void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid);
void deserialize_remove_entity(ENetPacket *packet, uint16_t &eid);
void deserialize_entity_input(ENetPacket *packet, uint16_t &eid, float &thr, float &steer);
void deserialize_sync_clock(ENetPacket *packet, int64_t &time);
void deserialize_snapshot(ENetPacket *packet, int64_t &time, uint16_t &eid,
                          float &x, float &y, float &ori, float &thr, float &steer);
