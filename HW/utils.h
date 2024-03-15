#ifndef UTILS_SENTRY
#define UTILS_SENTRY

#define WIN32_LEAN_AND_MEAN

#include <enet/enet.h>
#include <cstdio>

#define ARR_LEN(_arr) (sizeof(_arr)/sizeof((_arr)[0]))
#define STR_LEN(_str) (ARR_LEN(_str)-1)
#define VEC_BYTE_SIZE(_vec) ((_vec).size() * sizeof((_vec)[0]))

#define PACKET_IS_STAT_STRING(_packet, _str) \
    (ARR_LEN(_str) == (_packet)->dataLength && strcmp(_str, (const char *)(_packet)->data) == 0)

#define PACKET_IS_PREFIXED_BY_STAT_STRING(_packet, _str) \
    (ARR_LEN(_str) == (_packet)->dataLength && strstr(_str, (const char *)(_packet)->data) == _str)

#define MAKE_SPRINTF_BUF(_bufname, _bufsizename, _bufcap, _fmt, ...)     \
    char _bufname[_bufcap];                                                      \
    size_t _bufsizename = snprintf(_bufname, _bufcap, _fmt, ## __VA_ARGS__) + 1

inline bool packet_is_string(const ENetPacket *packet)
{
    return packet->data[packet->dataLength-1] == '\0';
}

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
}

#endif
