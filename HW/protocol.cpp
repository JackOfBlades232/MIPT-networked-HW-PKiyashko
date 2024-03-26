#include "protocol.h"
#include <cstring> // memcpy

void send_join(ENetPeer *peer)
{
    ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t), ENET_PACKET_FLAG_RELIABLE);
    *packet->data = e_client_to_server_join;

    enet_peer_send(peer, 0, packet);
}

void send_new_entity(ENetPeer *peer, const entity_t &ent)
{
    ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(entity_t), 
                                            ENET_PACKET_FLAG_RELIABLE);
    uint8_t *ptr = packet->data;
    *ptr = e_server_to_client_new_entity; ptr += sizeof(uint8_t);
    memcpy(ptr, &ent, sizeof(entity_t)); ptr += sizeof(entity_t);

    enet_peer_send(peer, 0, packet);
}

void send_set_controlled_entity(ENetPeer *peer, uint16_t eid)
{
    ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t), 
                                            ENET_PACKET_FLAG_RELIABLE);
    uint8_t *ptr = packet->data;
    *ptr = e_server_to_client_set_controlled_entity; ptr += sizeof(uint8_t);
    memcpy(ptr, &eid, sizeof(uint16_t)); ptr += sizeof(uint16_t);

    enet_peer_send(peer, 0, packet);
}

void send_entity_state(ENetPeer *peer, uint16_t eid, float x, float y)
{
    ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) + 2 * sizeof(float),
                                            ENET_PACKET_FLAG_UNSEQUENCED);
    uint8_t *ptr = packet->data;
    *ptr = e_client_to_server_state; ptr += sizeof(uint8_t);
    memcpy(ptr, &eid, sizeof(uint16_t)); ptr += sizeof(uint16_t);
    memcpy(ptr, &x, sizeof(float)); ptr += sizeof(float);
    memcpy(ptr, &y, sizeof(float)); ptr += sizeof(float);

    enet_peer_send(peer, 1, packet);
}

void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y)
{
    ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) + 2 * sizeof(float),
                                            ENET_PACKET_FLAG_UNSEQUENCED);
    uint8_t *ptr = packet->data;
    *ptr = e_server_to_client_snapshot; ptr += sizeof(uint8_t);
    memcpy(ptr, &eid, sizeof(uint16_t)); ptr += sizeof(uint16_t);
    memcpy(ptr, &x, sizeof(float)); ptr += sizeof(float);
    memcpy(ptr, &y, sizeof(float)); ptr += sizeof(float);

    enet_peer_send(peer, 1, packet);
}

message_type_t get_packet_type(ENetPacket *packet)
{
    return (message_type_t)*packet->data;
}

void deserialize_new_entity(ENetPacket *packet, entity_t &ent)
{
    uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
    ent = *(entity_t *)(ptr); ptr += sizeof(entity_t);
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid)
{
    uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
    eid = *(uint16_t *)(ptr); ptr += sizeof(uint16_t);
}

void deserialize_entity_state(ENetPacket *packet, uint16_t &eid, float &x, float &y)
{
    uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
    eid = *(uint16_t *)(ptr); ptr += sizeof(uint16_t);
    x = *(float *)(ptr); ptr += sizeof(float);
    y = *(float *)(ptr); ptr += sizeof(float);
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y)
{
    uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
    eid = *(uint16_t *)(ptr); ptr += sizeof(uint16_t);
    x = *(float *)(ptr); ptr += sizeof(float);
    y = *(float *)(ptr); ptr += sizeof(float);
}

