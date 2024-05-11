#pragma once

#include "tetrion.hpp"
#include <cstdint>
#include <sockets/sockets.hpp>
#include <vector>

struct ClientInfo final {
    std::uint8_t id;
    Tetrion tetrion;
    std::uint64_t num_frames_simulated{ 0 };
    std::vector<ObpfEvent> event_buffer;
};

class Server final {
private:
    c2k::ClientSocket m_lobby_socket;
    c2k::ServerSocket m_server_socket;
    std::vector<c2k::ClientSocket> m_client_sockets;
    c2k::Synchronized<std::vector<ClientInfo>> m_client_infos;
    std::vector<std::jthread> m_client_threads;
    std::jthread m_broadcasting_thread;
    std::size_t m_expected_player_count;
    std::uint8_t m_next_client_id = 0;
    std::atomic_flag m_should_stop;

public:
    explicit Server(std::uint16_t const lobby_port)
        : m_lobby_socket{ c2k::Sockets::create_client(c2k::AddressFamily::Ipv4, "127.0.0.1", lobby_port) },
          m_server_socket{ c2k::Sockets::create_server(
                  c2k::AddressFamily::Ipv4,
                  0,
                  [this](c2k::ClientSocket client) { accept_client_connection(std::move(client)); }
          ) },
          m_client_infos{ {} },
          m_broadcasting_thread{ keep_broadcasting, std::ref(*this) } {
        // todo: timeout
        m_expected_player_count = static_cast<std::size_t>(m_lobby_socket.receive<std::uint16_t>().get());

        m_client_sockets.reserve(m_expected_player_count);
        m_client_infos.apply([this](std::vector<ClientInfo>& client_infos) {
            client_infos.reserve(m_expected_player_count);
        });
        m_client_threads.reserve(m_expected_player_count);

        if (m_lobby_socket.send(m_server_socket.local_address().port).get() != 2) {
            throw std::runtime_error{ "unable to send port to lobby server" };
        }
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
            client_infos.emplace_back(client_id);

            assert(m_client_threads.size() == index);
            m_client_threads.emplace_back(process_client, std::ref(*this), index);
        });
    }

    static void process_client(std::stop_token const& stop_token, Server& self, std::size_t index);

    static void keep_broadcasting(std::stop_token const& stop_token, Server& self);
};
