#ifndef UTILS_SENTRY
#define UTILS_SENTRY

#define WIN32_LEAN_AND_MEAN

#include <enet/enet.h>
#include <vector>

#define ARR_LEN(_arr) (sizeof(_arr)/sizeof((_arr)[0]))
#define VEC_BYTE_SIZE(_vec) ((_vec).size() * sizeof((_vec)[0]))

// @TODO: improve, this seems strange
inline void send_packet(ENetPeer *peer, void *data, size_t data_size, bool is_reliable, bool is_small)
{
    enet_uint32 channel = is_reliable ? 0 : 1;

    enet_uint32 flags = 0;
    if (is_reliable)
        flags |= ENET_PACKET_FLAG_RELIABLE;
    else if (is_small)
        flags |= ENET_PACKET_FLAG_UNSEQUENCED;
    else
        flags |= ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT;

    ENetPacket *packet = enet_packet_create(data, data_size, flags);
    enet_peer_send(peer, channel, packet);
    enet_packet_destroy(packet);
}

#endif
