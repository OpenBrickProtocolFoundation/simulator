#pragma once

#include <lib2k/static_vector.hpp>
#include <network/constants.hpp>
#include <network/messages.hpp>
#include <sockets/sockets.hpp>
#include <string>
#include "observer_tetrion.hpp"
#include "tetrion.hpp"

template<typename T>
using NullableUniquePointer = std::unique_ptr<T>;

struct MultiplayerTetrion final : ObpfTetrion {
private:
    c2k::ClientSocket m_socket;
    std::uint8_t m_client_id;
    std::uint64_t m_start_frame;
    c2k::StaticVector<KeyState, heartbeat_interval> m_key_state_buffer;
    std::jthread m_receiving_thread;
    std::vector<std::unique_ptr<ObserverTetrion>> m_observers;
    c2k::Synchronized<std::deque<StateBroadcast>> m_broadcast_queue{ {} };

    struct Key {};

public:
    static NullableUniquePointer<MultiplayerTetrion> create(std::string const& server, std::uint16_t port);

    // we need address stability of the members here
    MultiplayerTetrion(MultiplayerTetrion const& other) = delete;
    MultiplayerTetrion(MultiplayerTetrion&& other) noexcept = delete;
    MultiplayerTetrion& operator=(MultiplayerTetrion const& other) = delete;
    MultiplayerTetrion& operator=(MultiplayerTetrion&& other) noexcept = delete;

    explicit MultiplayerTetrion(
        c2k::ClientSocket socket,
        std::uint8_t const client_id,
        std::uint64_t const start_frame,
        std::uint64_t const seed,
        std::vector<std::unique_ptr<ObserverTetrion>> observers,
        Key
    )
        : ObpfTetrion{ seed },
          m_socket{ std::move(socket) },
          m_client_id{ client_id },
          m_start_frame{ start_frame },
          m_receiving_thread{ keep_receiving, std::ref(m_socket), std::ref(m_broadcast_queue) },
          m_observers{ std::move(observers) } {}

    void simulate_next_frame(KeyState key_state) override;
    [[nodiscard]] std::vector<ObserverTetrion*> get_observers() const;

private:
    void send_heartbeat_message();
    void process_state_broadcast_message(StateBroadcast const& message);

    static void keep_receiving(
        std::stop_token const& stop_token,
        c2k::ClientSocket& socket,
        c2k::Synchronized<std::deque<StateBroadcast>>& queue
    );
};
