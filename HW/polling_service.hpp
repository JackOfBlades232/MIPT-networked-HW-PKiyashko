#pragma once

#include <enet/enet.h>

#include <cassert>
#include <deque>
#include <thread>
#include <mutex>

class PollingService {
    ENetHost *m_host = nullptr;

    std::deque<ENetEvent> m_event_q;
    std::mutex m_mutex; // Lazy way

    std::jthread m_polling_thread;

public:
    PollingService(ENetHost *host) : m_host(host) {}

    PollingService(const PollingService &) = delete;
    PollingService &operator=(const PollingService &) = delete;

    PollingService(PollingService &&) = default;
    PollingService &operator=(PollingService &&) = default;

    void Run() {
        assert(m_host);
        m_polling_thread = std::jthread([this]() {
            for (;;) {
                ENetEvent event;
                while (enet_host_service(m_host, &event, 0) > 0) {
                    std::lock_guard lock(m_mutex);
                    m_event_q.push_back(event);
                }
            }
        });
    }

    std::deque<ENetEvent> FetchEvents() {
        std::lock_guard lock(m_mutex);
        return std::move(m_event_q);
    }
};
