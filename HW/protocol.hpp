#pragma once
#include "entity.hpp"

#include <enet/enet.h>

#include <cstdint>

enum message_type_t : uint8_t {
    e_client_to_server_join = 0,
    e_server_to_client_new_entity,
    e_server_to_client_set_controlled_entity,
    e_client_to_server_state,
    e_server_to_client_snapshot,
    e_server_to_client_remove_entity,
    e_server_to_client_score_update
};

void send_join(ENetPeer *peer);
void send_new_entity(ENetPeer *peer, const entity_t &ent);
void send_set_controlled_entity(ENetPeer *peer, uint16_t eid);
void send_remove_entity(ENetPeer *peer, uint16_t eid);
void send_entity_score_update(ENetPeer *peer, uint16_t eid, uint16_t score);
void send_entity_state(ENetPeer *peer, uint16_t eid, float x, float y);
void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, float rad);

message_type_t get_packet_type(ENetPacket *packet);

void deserialize_new_entity(ENetPacket *packet, entity_t &ent);
void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid);
void deserialize_remove_entity(ENetPacket *packet, uint16_t &eid);
void deserialize_entity_score_update(ENetPacket *packet, uint16_t &eid, uint16_t &score);
void deserialize_entity_state(ENetPacket *packet, uint16_t &eid, float &x, float &y);
void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y, float &rad);
