#pragma once

#include <sockets/sockets.hpp>
#include <cstdint>
#include <vector>

struct ClientInfo final {
    std::uint8_t id;
    c2k::ClientSocket connection;

    ClientInfo(std::uint8_t const id, c2k::ClientSocket connection)
        : id{ id }
        , connection{ std::move(connection) } { }
};

class Server final {
private:
    c2k::ClientSocket m_lobby_socket;
    c2k::ServerSocket m_server_socket;
    std::vector<ClientInfo> m_clients;
    std::size_t m_expected_player_count;
    std::uint8_t m_next_client_id = 0;

public:
    explicit Server(std::uint16_t const lobby_port)
        : m_lobby_socket{
            c2k::Sockets::create_client(
                c2k::AddressFamily::Ipv4,
                "127.0.0.1",
                lobby_port
            )
        }
        , m_server_socket{
            c2k::Sockets::create_server(
                c2k::AddressFamily::Ipv4,
                0,
                [this](c2k::ClientSocket client) {
                    accept_client_connection(std::move(client));
                }
            )
        } {
        auto received = c2k::Extractor{};
        while (true) {
            received << m_lobby_socket.receive(2 - received.size()).get();
            if (received.size() >= 2) {
                break;
            }
        }
        assert(received.size() == 2);
        m_expected_player_count = static_cast<std::size_t>(received.try_extract<std::uint16_t>().value());

        if (m_lobby_socket.send(m_server_socket.local_address().port).get() != 2) {
            throw std::runtime_error{ "unable to send port to lobby server" };
        }
    }

private:
    void accept_client_connection(c2k::ClientSocket client) {
        if (m_clients.size() >= m_expected_player_count) {
            // reject client
            return;
        }
        auto const client_id = m_next_client_id++;
        m_clients.emplace_back(client_id, std::move(client));
    }
};
