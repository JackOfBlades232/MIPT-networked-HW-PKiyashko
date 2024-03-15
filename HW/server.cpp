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

static void send_new_player_packet(ENetHost &host, const player_t &new_player, const ENetPeer *new_peer)
{
    MAKE_SPRINTF_BUF(buf, buf_len, 64, "newplayer:%llu:%s", new_player.hash, new_player.name.c_str());
    for (size_t i = 0; i < host.connectedPeers; ++i) {
        ENetPeer *peer = &host.peers[i];
        if (peer != new_peer)
            send_packet(peer, buf, buf_len, true, true);
    }
}

static void send_all_players_packet(ENetPeer *peer,
                                    ENetHost &host, const std::map<ENetPeer *, player_t> &players)
{
    std::string msg("playerlist");
    for (size_t i = 0; i < host.connectedPeers; ++i) {
        ENetPeer *other_peer = &host.peers[i];
        if (other_peer != peer) {
            const player_t &other_player = players.at(other_peer);
            MAKE_SPRINTF_BUF(buf, buf_len, 64, ":%llu:%s", other_player.hash, other_player.name.c_str());
            msg += std::string(buf, buf_len-1);
        }
    }

    send_packet(peer, (void *)msg.c_str(), msg.length()+1, true, false);
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

    uint32_t time_start = enet_time_get();
    uint32_t last_pings_sent_time = time_start;
    while (true) {
        ENetEvent event;
        while (enet_host_service(server, &event, 10) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
            { 
                printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
                uint64_t hash = hash_client_id(server->connectedPeers-1, hash_offset);
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
        if (cur_time - last_pings_sent_time > 500) {
            // @TODO: send all pings to everyone (pings:%hash:%ping)
        }
    }

    enet_host_destroy(server);

    atexit(enet_deinitialize);
    return 0;
}

