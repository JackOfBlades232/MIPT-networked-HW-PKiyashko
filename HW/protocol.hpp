#pragma once
#include "entity.hpp"

#include <enet/enet.h>

#include <cstdint>

enum message_type_t : uint8_t {
    e_client_to_server_join = 0,
    e_server_to_client_new_entity,
    e_server_to_client_set_controlled_entity,
    e_server_to_client_remove_entity,
    e_client_to_server_input,
    e_server_to_client_snapshot
};

void send_join(ENetPeer *peer);
void send_new_entity(ENetPeer *peer, const entity_t &ent);
void send_set_controlled_entity(ENetPeer *peer, uint16_t eid);
void send_remove_entity(ENetPeer *peer, uint16_t eid);
void send_entity_input(ENetPeer *peer, uint16_t eid, float thr, float steer);
void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, float ori);

message_type_t get_packet_type(ENetPacket *packet);

void deserialize_new_entity(ENetPacket *packet, entity_t &ent);
void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid);
void deserialize_remove_entity(ENetPacket *packet, uint16_t &eid);
void deserialize_entity_input(ENetPacket *packet, uint16_t &eid, float &thr, float &steer);
void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y, float &ori);
