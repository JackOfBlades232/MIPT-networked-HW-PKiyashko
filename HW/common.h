#ifndef COMMON_SENTRY
#define COMMON_SENTRY

#include <enet/enet.h>

struct player_t {
    uint64_t hash;
    const char *name;
};

#endif
