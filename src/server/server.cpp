#include "server.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <chrono>
#include <gsl/gsl>
#include <numeric>
#include <ranges>
#include "network/messages.hpp"

void Server::process_client(std::stop_token const& stop_token, Server& self, std::size_t index) {
    using namespace std::chrono_literals;
    auto& socket = self.m_client_sockets.at(index);

    while (not stop_token.stop_requested() and socket.is_connected()) {
        auto message = std::unique_ptr<AbstractMessage>{};
        try {
            message = AbstractMessage::from_socket(socket);
        } catch (c2k::TimeoutError const& exception) {
            spdlog::error("timeout error while waiting for next client message: {}", exception.what());
            continue;  // todo: require clients to send heartbeats even before the game starts
        } catch (c2k::ReadError const& exception) {
            spdlog::error("error while reading from socket: {}", exception.what());
            break;
        }
        if (message->type() == MessageType::Heartbeat) {
            spdlog::info("received heartbeat message");
            auto const& heartbeat_message = dynamic_cast<Heartbeat const&>(*message);
            // todo: check whether data is valid

            spdlog::info("heartbeat message frame: {}", heartbeat_message.frame);

            self.m_client_infos.apply([&heartbeat_message, index](std::vector<ClientInfo>& client_infos) {
                auto& client_info = client_infos.at(index);

                auto& tetrion = client_info.tetrion;
                for (auto const key_state : heartbeat_message.key_states) {
                    client_info.key_states.push_back(key_state);
                    tetrion.simulate_next_frame(key_state);
                }
            });
        }
    }
    spdlog::info("client {}:{} disconnected", socket.remote_address().address, socket.remote_address().port);
}

// clang-format off
[[nodiscard]] static c2k::MessageBuffer create_broadcast_message(
    std::vector<ClientInfo> & client_infos,
    std::uint64_t const frame
) {  // clang-format on
    spdlog::info("creating broadcast message for frame {}", frame);

    auto client_states = std::vector<StateBroadcast::ClientStates>{};
    for (auto& client_info : client_infos) {
        assert(client_info.key_states.size() >= heartbeat_interval);
        auto states = std::array<KeyState, heartbeat_interval>{};
        std::copy_n(client_info.key_states.cbegin(), heartbeat_interval, states.begin());
        client_info.key_states.erase(client_info.key_states.begin(), client_info.key_states.begin() + heartbeat_interval);
        client_states.emplace_back(client_info.id, states);
    }

    auto const broadcast_message = StateBroadcast{ frame, std::move(client_states) };
    return broadcast_message.serialize();
}

void Server::keep_broadcasting(std::stop_token const& stop_token, Server& self) {
    using namespace std::chrono_literals;

    // wait for all clients to connect
    while (not stop_token.stop_requested()) {
        auto const num_connected_clients =
            self.m_client_infos.apply([](std::vector<ClientInfo> const& client_infos) { return client_infos.size(); });
        if (num_connected_clients == self.m_expected_player_count) {
            break;
        }
        spdlog::info("not all clients have connected yet, number of clients: {}", num_connected_clients);
        // todo: replace sleep with condition variable
        std::this_thread::sleep_for(100ms);
    }

    auto i = std::uint8_t{ 0 };
    for (auto& socket : self.m_client_sockets) {
        spdlog::info("assigning id {} to client and sending seed {}", i, self.m_seed);
        auto const message = GameStart{
            i,
            180,
            self.m_seed,
            gsl::narrow<std::uint8_t>(self.m_client_sockets.size()),
        };
        socket.send(message.serialize()).wait();
        ++i;
    }

    auto last_min_num_frames_simulated = std::optional<std::uint64_t>{ std::nullopt };

    while (not stop_token.stop_requested()) {
        self.m_client_infos.apply([&self, &last_min_num_frames_simulated](std::vector<ClientInfo>& client_infos) {
            auto const min_num_frames_simulated = std::min_element(
                client_infos.cbegin(),
                client_infos.cend(),
                [](ClientInfo const& lhs, ClientInfo const& rhs) {
                    return lhs.tetrion.next_frame() < rhs.tetrion.next_frame();
                }
            );
            auto const frame = min_num_frames_simulated->tetrion.next_frame();
            if (frame == 0) {
                return;
            }

            if (not last_min_num_frames_simulated.has_value() or frame != last_min_num_frames_simulated.value()) {
                auto const broadcast_message = create_broadcast_message(client_infos, frame - 1);
                for (auto& socket : self.m_client_sockets) {
                    std::ignore = socket.send(broadcast_message);
                }
                last_min_num_frames_simulated = frame;
            }
        });

        // todo: replace sleep with condition variable
        std::this_thread::sleep_for(100ms);
    }
}
