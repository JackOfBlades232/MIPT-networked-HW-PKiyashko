#include "utils.h"
#include "common.h"
#include <enet/enet.h>
#include <cstdlib>
#include <ctime>
#include <map>
#include <string>

static const char *egyptian_names[32] =
{
 "Mohamed",
 "Youssef",
 "Ahmed",
 "Mahmoud",
 "Mustafa",
 "Yassin",
 "Taha",
 "Khaled",
 "Hamza",
 "Bilal",
 "Ibrahim",
 "Hassan",
 "Hussein",
 "Karim",
 "Tareq",
 "Abdel-Rahman",
 "Ali",
 "Omar",
 "Halim",
 "Murad",
 "Selim",
 "Abdallah"
 "Peter",
 "Pierre",
 "George",
 "John",
 "Mina",
 "Beshoi",
 "Kirollos",
 "Mark",
 "Fadi",
 "Habib"
};

static uint64_t hash_client_id(size_t client_index, uint64_t offset)
{
    return (uint64_t)client_index * 0x123211123 + offset; // mult odd => creates a 1to1 map of Z/2^64 Z
}

static const char *get_client_name(uint64_t client_hash)
{
    return egyptian_names[client_hash % ARR_LEN(egyptian_names)];
}

// @NOTE: this could be factored out in some sane way
#define BEGIN_PEER_LOOP(_host, _peer_name, _this_peer)      \
    size_t _processed_peers = 0;                            \
    for (size_t i = 0; i < _host.peerCount; ++i) {          \
        ENetPeer *_peer_name = &_host.peers[i];             \
        if (_peer_name->state != ENET_PEER_STATE_CONNECTED) \
            continue;                                       \
        if (_peer_name != _this_peer) {

#define END_PEER_LOOP(_host)                            \
        }                                               \
        if (++_processed_peers >= _host.connectedPeers) \
            break;                                      \
    }

static void send_new_player_packet(ENetHost &host, const player_t &new_player, const ENetPeer *new_peer)
{
    MAKE_SPRINTF_BUF(buf, buf_len, 64, "newplayer:%llu:%s", new_player.hash, new_player.name.c_str());
    BEGIN_PEER_LOOP(host, peer, new_peer)
        send_packet(peer, buf, buf_len, true, true);
    END_PEER_LOOP(host)
}

static void send_all_players_packet(ENetPeer *peer,
                                    ENetHost &host, const std::map<ENetPeer *, player_t> &players)
{
    std::string msg("playerlist");
    BEGIN_PEER_LOOP(host, other_peer, peer)
        const player_t &other_player = players.at(other_peer);
        MAKE_SPRINTF_BUF(buf, buf_len, 64, ":%llu:%s", other_player.hash, other_player.name.c_str());
        msg += std::string(buf, buf_len-1);
    END_PEER_LOOP(host)

    send_packet(peer, (void *)msg.c_str(), msg.length()+1, true, false);
}

static void send_all_pings_packet(ENetPeer *peer,
                                  ENetHost &host, const std::map<ENetPeer *, player_t> &players)
{
    std::string msg("pings");
    BEGIN_PEER_LOOP(host, other_peer, peer)
        const player_t &other_player = players.at(other_peer);
        MAKE_SPRINTF_BUF(buf, buf_len, 64, ":%s:%u", other_player.name.c_str(), other_peer->lastRoundTripTime);
        msg += std::string(buf, buf_len-1);
    END_PEER_LOOP(host)

    send_packet(peer, (void *)msg.c_str(), msg.length()+1, false, false);
}

static void send_pings_to_all_players(ENetHost &host, const std::map<ENetPeer *, player_t> &players)
{
    BEGIN_PEER_LOOP(host, peer, nullptr)
        send_all_pings_packet(peer, host, players);
    END_PEER_LOOP(host)
}

int main(int argc, const char **argv)
{
    srand(time(nullptr));
    const uint64_t hash_offset = rand();
    
    if (enet_initialize() != 0) {
        printf("Cannot init ENet");
        return 1;
    }
    ENetAddress address;

    address.host = ENET_HOST_ANY;
    address.port = 4221;

    ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);
    if (!server) {
        printf("Cannot create ENet server\n");
        return 1;
    }

    std::map<ENetPeer *, player_t> players;
    size_t player_counter = 0;

    uint32_t time_start = enet_time_get();
    uint32_t last_pings_sent_time = time_start;
    while (true) {
        ENetEvent event;
        while (enet_host_service(server, &event, 10) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
            { 
                printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
                uint64_t hash = hash_client_id(player_counter++, hash_offset);
                player_t new_player{hash, std::string(get_client_name(hash))};
                players.emplace(event.peer, new_player);
                send_new_player_packet(*server, new_player, event.peer);
                send_all_players_packet(event.peer, *server, players);
            } break;
            case ENET_EVENT_TYPE_RECEIVE:
            {
                const player_t &player_data = players.at(event.peer);
                printf("Packet received from #%llu(%s) '%s'\n", player_data.hash, player_data.name.c_str(), event.packet->data);
                enet_packet_destroy(event.packet);
            } break;
            case ENET_EVENT_TYPE_DISCONNECT:
                printf("Peer %x:%u disconnected\n", event.peer->address.host, event.peer->address.port);
                players.erase(event.peer);
                break;
            default:
                break;
            };
        }

        uint32_t cur_time = enet_time_get();
        if (cur_time - last_pings_sent_time > 250) {
            send_pings_to_all_players(*server, players);
            last_pings_sent_time = cur_time;
        }
    }

    enet_host_destroy(server);

    atexit(enet_deinitialize);
    return 0;
}

