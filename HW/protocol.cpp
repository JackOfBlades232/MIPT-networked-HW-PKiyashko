#include "protocol.hpp"
#include "bitstream.hpp"
#include "enet/enet.h"
#include <cstdint>

static Bitstream create_packet_writer_bs(ENetPacket *packet)
{
    return Bitstream(packet->data, packet->dataLength, Bitstream::Type::eWriter);
}

static Bitstream create_packet_reader_bs(ENetPacket *packet)
{
    return Bitstream(packet->data, packet->dataLength, Bitstream::Type::eReader);
}

void send_join(ENetPeer *peer)
{
    if (peer->state != ENET_PEER_STATE_CONNECTED)
        return;

    ENetPacket *packet = enet_packet_create(nullptr, sizeof(message_type_t), ENET_PACKET_FLAG_RELIABLE);
    *packet->data = e_client_to_server_join;

    enet_peer_send(peer, 0, packet);
}

void send_disconnect(ENetPeer *peer)
{
    if (peer->state != ENET_PEER_STATE_CONNECTED)
        return;

    ENetPacket *packet = enet_packet_create(nullptr, sizeof(message_type_t), ENET_PACKET_FLAG_RELIABLE);
    *packet->data = e_client_to_server_disconnect;

    enet_peer_send(peer, 0, packet);
}

void send_new_entity(ENetPeer *peer, const entity_t &ent)
{
    if (peer->state != ENET_PEER_STATE_CONNECTED)
        return;

    ENetPacket *packet = enet_packet_create(nullptr, sizeof(message_type_t) + sizeof(ent), 
                                            ENET_PACKET_FLAG_RELIABLE);
    Bitstream bs = create_packet_writer_bs(packet);
    bs.Write(e_server_to_client_new_entity);
    bs.Write(ent);

    enet_peer_send(peer, 0, packet);
}

void send_set_controlled_entity(ENetPeer *peer, uint16_t eid)
{
    if (peer->state != ENET_PEER_STATE_CONNECTED)
        return;

    ENetPacket *packet = enet_packet_create(nullptr, sizeof(message_type_t) + sizeof(eid), 
                                            ENET_PACKET_FLAG_RELIABLE);
    Bitstream bs = create_packet_writer_bs(packet);
    bs.Write(e_server_to_client_set_controlled_entity);
    bs.Write(eid);

    enet_peer_send(peer, 0, packet);
}

void send_remove_entity(ENetPeer *peer, uint16_t eid)
{
    if (peer->state != ENET_PEER_STATE_CONNECTED)
        return;

    ENetPacket *packet = enet_packet_create(nullptr, sizeof(message_type_t) + sizeof(eid), 
                                            ENET_PACKET_FLAG_RELIABLE);
    Bitstream bs = create_packet_writer_bs(packet);
    bs.Write(e_server_to_client_remove_entity);
    bs.Write(eid);

    enet_peer_send(peer, 0, packet);
}

void send_entity_input(ENetPeer *peer, uint16_t eid, int64_t time, float thr, float steer)
{
    if (peer->state != ENET_PEER_STATE_CONNECTED)
        return;

    ENetPacket *packet = enet_packet_create(nullptr, 
                                            sizeof(message_type_t) + sizeof(eid) +
                                            sizeof(time) + sizeof(thr) + sizeof(steer),
                                            ENET_PACKET_FLAG_RELIABLE);
    Bitstream bs = create_packet_writer_bs(packet);
    bs.Write(e_client_to_server_input);
    bs.Write(eid);
    bs.Write(time);
    bs.Write(thr);
    bs.Write(steer);

    enet_peer_send(peer, 1, packet);
}

void send_snapshot(ENetPeer *peer, int64_t time, uint16_t eid, float x,
                   float y, float ori, float thr, float steer)
{
    if (peer->state != ENET_PEER_STATE_CONNECTED)
        return;

    ENetPacket *packet = enet_packet_create(nullptr,
                                            sizeof(message_type_t) + sizeof(time) +
                                            sizeof(eid) + sizeof(x) + sizeof(y) + sizeof(ori) +
                                            sizeof(thr) + sizeof(steer),
                                            ENET_PACKET_FLAG_UNSEQUENCED);

    Bitstream bs = create_packet_writer_bs(packet);
    bs.Write(e_server_to_client_snapshot);
    bs.Write(time);
    bs.Write(eid);
    bs.Write(x);
    bs.Write(y);
    bs.Write(ori);
    bs.Write(thr);
    bs.Write(steer);

    enet_peer_send(peer, 1, packet);
}

message_type_t get_packet_type(ENetPacket *packet)
{
    return (message_type_t)*packet->data;
}

void deserialize_new_entity(ENetPacket *packet, entity_t &ent)
{
    Bitstream bs = create_packet_reader_bs(packet);
    bs.Skip<message_type_t>();
    bs.Read(ent);
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid)
{
    Bitstream bs = create_packet_reader_bs(packet);
    bs.Skip<message_type_t>();
    bs.Read(eid);
}

void deserialize_remove_entity(ENetPacket *packet, uint16_t &eid)
{
    Bitstream bs = create_packet_reader_bs(packet);
    bs.Skip<message_type_t>();
    bs.Read(eid);
}

void deserialize_entity_input(ENetPacket *packet, uint16_t &eid, int64_t &time, float &thr, float &steer)
{
    Bitstream bs = create_packet_reader_bs(packet);
    bs.Skip<message_type_t>();
    bs.Read(eid);
    bs.Read(time);
    bs.Read(thr);
    bs.Read(steer);
}

void deserialize_snapshot(ENetPacket *packet, int64_t &time, uint16_t &eid,
                          float &x, float &y, float &ori, float &thr, float &steer)
{
    Bitstream bs = create_packet_reader_bs(packet);
    bs.Skip<message_type_t>();
    bs.Read(time);
    bs.Read(eid);
    bs.Read(x);
    bs.Read(y);
    bs.Read(ori);
    bs.Read(thr);
    bs.Read(steer);
}

