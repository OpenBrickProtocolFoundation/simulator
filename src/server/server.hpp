#pragma once

#include <spdlog/spdlog.h>
#include <atomic>
#include <cstdint>
#include <lib2k/random.hpp>
#include <simulator/tetrion.hpp>
#include <sockets/sockets.hpp>
#include <vector>

enum class ClientState {
    Connected,
    Identified,
    Disconnected,
};

struct ClientInfo final {
    u8 id;
    ObpfTetrion tetrion;
    std::vector<KeyState> key_states;
    ClientState state = ClientState::Connected;
    std::string player_name;  // Not filled by constructor, because the name is transferred later.

    explicit ClientInfo(u8 const id, u64 const seed, u64 const start_frame)
        : id{ id }, tetrion{ seed, start_frame } {}

    [[nodiscard]] bool is_connected() const {
        switch (state) {
            using enum ClientState;
            case Connected:
            case Identified:
                return true;
            case Disconnected:
                return false;
        }
        throw std::logic_error{ "unreachable" };
    }
};

class Server final {
private:
    std::optional<c2k::ClientSocket> m_lobby_socket;
    c2k::ServerSocket m_server_socket;
    std::vector<c2k::ClientSocket> m_client_sockets;
    c2k::Synchronized<std::vector<ClientInfo>> m_client_infos;
    std::atomic_size_t m_expected_player_count = 0;
    std::vector<std::jthread> m_client_threads;
    std::jthread m_broadcasting_thread;
    std::uint8_t m_next_client_id = 0;
    std::atomic_flag m_should_stop;
    c2k::Random::Seed m_seed;

    static constexpr auto start_frame = u64{ 180 };

public:
    explicit Server(std::uint16_t const lobby_port)
        : m_lobby_socket{ c2k::Sockets::create_client(c2k::AddressFamily::Ipv4, "127.0.0.1", lobby_port) },
          m_server_socket{ c2k::Sockets::create_server(
              c2k::AddressFamily::Ipv4,
              0,
              [this](c2k::ClientSocket client) { accept_client_connection(std::move(client)); }
          ) },
          m_client_infos{ {} },
          m_broadcasting_thread{ keep_broadcasting, std::ref(*this) },
          m_seed{ c2k::Random{}.next_integral<c2k::Random::Seed>() } {
        // todo: timeout
        m_expected_player_count = static_cast<std::size_t>(m_lobby_socket.value().receive<std::uint16_t>().get());
        spdlog::info("expected player count: {}", m_expected_player_count.load());

        m_client_sockets.reserve(m_expected_player_count);
        m_client_infos.apply([this](std::vector<ClientInfo>& client_infos) {
            client_infos.reserve(m_expected_player_count);
        });
        m_client_threads.reserve(m_expected_player_count);

        if (m_lobby_socket.value().send(m_server_socket.local_address().port).get() != sizeof(std::uint16_t)) {
            throw std::runtime_error{ "unable to send port to lobby server" };
        }
    }

    explicit Server(std::uint16_t const game_server_port, std::uint8_t const num_expected_players)
        : m_server_socket{ c2k::Sockets::create_server(
              c2k::AddressFamily::Ipv4,
              game_server_port,
              [this](c2k::ClientSocket client) { accept_client_connection(std::move(client)); }
          ) },
          m_client_infos{ {} },
          m_broadcasting_thread{ keep_broadcasting, std::ref(*this) },
          m_seed{ c2k::Random{}.next_integral<c2k::Random::Seed>() } {
        // todo: timeout
        m_expected_player_count = num_expected_players;
        spdlog::info("expected player count: {}", m_expected_player_count.load());

        m_client_sockets.reserve(m_expected_player_count);
        m_client_infos.apply([this](std::vector<ClientInfo>& client_infos) {
            client_infos.reserve(m_expected_player_count);
        });
        m_client_threads.reserve(m_expected_player_count);
    }

    Server(Server const&) = delete;
    Server(Server&&) noexcept = delete;
    Server& operator=(Server const&) = delete;
    Server& operator=(Server&&) = delete;

    ~Server() {
        m_should_stop.wait(false);
    }

    void stop() {
        if (not m_should_stop.test_and_set()) {
            m_should_stop.notify_one();
        }
    }

private:
    void accept_client_connection(c2k::ClientSocket client) {
        m_client_infos.apply([this, client = std::move(client)](std::vector<ClientInfo>& client_infos) mutable {
            if (client_infos.size() >= m_expected_player_count) {
                // reject client
                return;
            }
            auto const index = m_client_sockets.size();
            m_client_sockets.push_back(std::move(client));

            assert(client_infos.size() == index);
            auto const client_id = m_next_client_id++;
            client_infos.emplace_back(client_id, m_seed, start_frame);

            assert(m_client_threads.size() == index);
            m_client_threads.emplace_back(process_client, std::ref(*this), index);
        });
    }

    void broadcast_client_disconnected_message(u8 client_id);

    static void process_client(std::stop_token const& stop_token, Server& self, std::size_t index);

    static void keep_broadcasting(std::stop_token const& stop_token, Server& self);
};
