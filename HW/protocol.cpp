#include "protocol.hpp"
#include "bitstream.hpp"

#include <utility>

static Bitstream create_packet_writer_bs(ENetPacket *packet)
{
    return Bitstream(packet->data, packet->dataLength, Bitstream::Type::writer);
}

static Bitstream create_packet_reader_bs(ENetPacket *packet)
{
    return Bitstream(packet->data, packet->dataLength, Bitstream::Type::reader);
}

void send_join(ENetPeer *peer)
{
    if (peer->state != ENET_PEER_STATE_CONNECTED)
        return;

    ENetPacket *packet = enet_packet_create(nullptr, sizeof(message_type_t), ENET_PACKET_FLAG_RELIABLE);
    *packet->data = e_client_to_server_join;

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

void send_entity_score_update(ENetPeer *peer, uint16_t eid, uint16_t score)
{
    if (peer->state != ENET_PEER_STATE_CONNECTED)
        return;

    ENetPacket *packet = enet_packet_create(nullptr, 
                                            sizeof(message_type_t) + sizeof(eid) + sizeof(score),
                                            ENET_PACKET_FLAG_RELIABLE);
    Bitstream bs = create_packet_writer_bs(packet);
    bs.Write(e_server_to_client_score_update);
    bs.Write(eid);
    bs.Write(score);

    enet_peer_send(peer, 1, packet);
}

void send_entity_state(ENetPeer *peer, uint16_t eid, float x, float y)
{
    if (peer->state != ENET_PEER_STATE_CONNECTED)
        return;

    ENetPacket *packet = enet_packet_create(nullptr, 
                                            sizeof(message_type_t) + sizeof(eid) + sizeof(x) + sizeof(y),
                                            ENET_PACKET_FLAG_UNSEQUENCED);
    Bitstream bs = create_packet_writer_bs(packet);
    bs.Write(e_client_to_server_state);
    bs.Write(eid);
    bs.Write(x);
    bs.Write(y);

    enet_peer_send(peer, 1, packet);
}

void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, float rad)
{
    if (peer->state != ENET_PEER_STATE_CONNECTED)
        return;

    ENetPacket *packet = enet_packet_create(nullptr,
                                            sizeof(message_type_t) + sizeof(eid) +
                                            sizeof(x) + sizeof(y) + sizeof(rad),
                                            ENET_PACKET_FLAG_UNSEQUENCED);

    Bitstream bs = create_packet_writer_bs(packet);
    bs.Write(e_server_to_client_snapshot);
    bs.Write(eid);
    bs.Write(x);
    bs.Write(y);
    bs.Write(rad);

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

void deserialize_entity_score_update(ENetPacket *packet, uint16_t &eid, uint16_t &score)
{
    Bitstream bs = create_packet_reader_bs(packet);
    bs.Skip<message_type_t>();
    bs.Read(eid);
    bs.Read(score);
}

void deserialize_entity_state(ENetPacket *packet, uint16_t &eid, float &x, float &y)
{
    Bitstream bs = create_packet_reader_bs(packet);
    bs.Skip<message_type_t>();
    bs.Read(eid);
    bs.Read(x);
    bs.Read(y);
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y, float &rad)
{
    Bitstream bs = create_packet_reader_bs(packet);
    bs.Skip<message_type_t>();
    bs.Read(eid);
    bs.Read(x);
    bs.Read(y);
    bs.Read(rad);
}

