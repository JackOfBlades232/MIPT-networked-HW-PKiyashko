#ifndef COMMON_SENTRY
#define COMMON_SENTRY

#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <string>

struct player_t {
    uint64_t hash;
    std::string name;
};

struct player_ping_t {
    uint64_t hash;
    uint32_t ping;
};

inline size_t fetch_player_data_from_message(const char *msgp, player_t &out_player)
{
    const char *init_msgp = msgp;
    assert(msgp);
    if (*msgp != ':')
        return (size_t)(-1);
    ++msgp;
    char *endptr;
    out_player.hash = strtoull(msgp, &endptr, 10);
    if (!endptr || *endptr != ':')
        return (size_t)(-1);
    ++endptr;
    msgp = endptr;
    while (*endptr && *endptr != ':')
        ++endptr;
    out_player.name = std::string(msgp, endptr - msgp);
    return endptr - init_msgp;
}

inline size_t fetch_ping_data_from_message(const char *msgp, player_ping_t &out_ping)
{
    const char *init_msgp = msgp;
    assert(msgp);
    if (*msgp != ':')
        return (size_t)(-1);
    ++msgp;
    char *endptr;
    out_ping.hash = strtoull(msgp, &endptr, 10);
    if (!endptr || *endptr != ':')
        return (size_t)(-1);
    msgp = endptr+1;
    out_ping.ping = strtoul(msgp, &endptr, 10);
    if (!endptr || (*endptr && *endptr != ':'))
        return (size_t)(-1);
    return endptr - init_msgp;
}

#endif
