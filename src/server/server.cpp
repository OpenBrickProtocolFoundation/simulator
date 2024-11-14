#include "server.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <chrono>
#include <gsl/gsl>
#include <lib2k/types.hpp>
#include <numeric>
#include <ranges>
#include "network/messages.hpp"

void Server::broadcast_client_disconnected_message(u8 const client_id) {
    auto const message = ClientDisconnected{ client_id };
    for (auto& socket : m_client_sockets) {
        if (socket.is_connected()) {
            spdlog::info("broadcasting disconnected client to socket");
            std::ignore = socket.send(message.serialize());
        }
    }
}

void Server::process_client(std::stop_token const& stop_token, Server& self, std::size_t const index) {
    using namespace std::chrono_literals;
    auto& socket = self.m_client_sockets.at(index);

    // Wait for Connect message from client...
    while (not stop_token.stop_requested() and socket.is_connected()) {
        auto message = std::unique_ptr<AbstractMessage>{};
        try {
            message = AbstractMessage::from_socket(socket);
        } catch (c2k::TimeoutError const&) {
            spdlog::error("Waiting for Connect message from client {}...", index);
            continue;
        } catch (c2k::ReadError const& exception) {
            spdlog::error("error while reading from socket: {}", exception.what());
            break;
        }

        if (message->type() == MessageType::Connect) {
            auto& connect_message = dynamic_cast<Connect&>(*message);
            spdlog::info("Client identified itself as '{}'.", connect_message.player_name);
            // clang-format off
            self.m_client_infos.apply(
                [index, name = std::move(connect_message.player_name)]
                (std::vector<ClientInfo>& client_infos) mutable {
                    client_infos.at(index).player_name = std::move(name);
                    assert(client_infos.at(index).state == ClientState::Connected);
                    client_infos.at(index).state = ClientState::Identified;
                }
            );
            // clang-format on
            break;
        }

        // todo: This client sent an unexpected message. It should be disconnected.
    }

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
                for (auto const key_state : heartbeat_message.key_states) {
                    // Queue all the key states and simulate the frames on the main thread when
                    // the states of all clients have arrived. We cannot simulate the tetrions right
                    // here because they can influence each other via sent garbage. This has to
                    // be synchronized.
                    client_infos.at(index).key_states.push_back(key_state);
                }
            });
        }
    }
    spdlog::info("client {}:{} disconnected", socket.remote_address().address, socket.remote_address().port);
    auto const client_id = self.m_client_infos.apply([index](std::vector<ClientInfo>& client_infos) {
        auto& client_info = client_infos.at(index);
        client_info.state = ClientState::Disconnected;
        return client_info.id;
    });
    self.broadcast_client_disconnected_message(client_id);
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

    while (self.m_expected_player_count == 0) {
        std::this_thread::sleep_for(10ms);
    }

    // Wait for all clients to connect and identify.
    while (not stop_token.stop_requested()) {
        // clang-format off
        auto const num_identified_clients = self.m_client_infos.apply(
            [](std::vector<ClientInfo> const& client_infos) {
                return gsl::narrow<usize>(
                    std::ranges::count_if(
                        client_infos,
                        [&](ClientInfo const& info) {
                            return info.state == ClientState::Identified;
                        }
                    )
                );
            }
        );
        // clang-format on
        if (num_identified_clients == self.m_expected_player_count) {
            break;
        }
        spdlog::info(
            "not all clients have connected/identified yet ({} of {})",
            num_identified_clients,
            self.m_expected_player_count.load()
        );
        // todo: replace sleep with condition variable
        std::this_thread::sleep_for(100ms);
    }

    auto const client_identities = self.m_client_infos.apply([](std::vector<ClientInfo>& client_infos) {
        auto identities = std::vector<ClientIdentity>{};
        identities.reserve(client_infos.size());
        for (auto const& [i, client_info] : std::views::enumerate(client_infos)) {
            identities.emplace_back(gsl::narrow<u8>(i), client_info.player_name);
        }
        return identities;
    });

    for (auto const& [i, socket] : std::views::enumerate(self.m_client_sockets)) {
        spdlog::info("assigning id {} to client and sending seed {}", i, self.m_seed);
        auto const message = GameStart{
            gsl::narrow<u8>(i),
            start_frame,
            self.m_seed,
            client_identities,
        };
        socket.send(message.serialize()).wait();
    }

    auto last_min_num_frames_simulated = std::optional<std::uint64_t>{ std::nullopt };

    while (not stop_token.stop_requested()) {
        auto const num_clients_connected =
            self.m_client_infos.apply([&self, &last_min_num_frames_simulated](std::vector<ClientInfo>& client_infos) {
                auto const num_clients_connected =
                    std::ranges::count_if(client_infos, [](ClientInfo const& client_info) {
                        return client_info.is_connected();
                    });

                if (num_clients_connected == 0) {
                    return num_clients_connected;
                }

                // go through all the connected clients and determine the minimum number of key states
                // that have been queued up for all clients
                auto const min_num_key_states_queued = std::ranges::min(
                    client_infos | std::views::filter([](auto const& client_info) { return client_info.is_connected(); })
                    | std::views::transform([](auto const& client_info) { return client_info.key_states.size(); })
                );

                for (auto i = usize{ 0 }; i < min_num_key_states_queued; ++i) {
                    auto garbage_send_events = std::unordered_map<u8, GarbageSendEvent>{};
                    for (auto& client_info : client_infos) {
                        if (not client_info.is_connected()) {
                            continue;
                        }
                        auto const key_state = client_info.key_states.at(i);
                        auto& tetrion = client_info.tetrion;
                        // clang-format off
                        if (
                            auto const garbage_send_event = tetrion.simulate_next_frame(key_state);
                            garbage_send_event.has_value()
                        ) {  // clang-format on
                            garbage_send_events.emplace(client_info.id, garbage_send_event.value());
                        }
                    }
                    auto const tetrions =
                        client_infos | std::views::transform([](auto& client_info) { return &client_info.tetrion; })
                        | std::ranges::to<std::vector>();
                    for (auto const [sender_client_id, garbage_send_event] : garbage_send_events) {
                        auto target_tetrion =
                            determine_garbage_target(tetrions, sender_client_id, garbage_send_event.frame);
                        if (target_tetrion.has_value()) {
                            target_tetrion.value().receive_garbage(garbage_send_event);
                        }
                    }
                }

                // first we need to find the minimum number of frames simulated by any client that is connected
                auto const min_num_frames_simulated = std::ranges::min(
                    client_infos | std::views::filter([](auto const& client_info) { return client_info.is_connected(); })
                    | std::views::transform([](auto const& client_info) { return client_info.tetrion.next_frame(); })
                );

                // to not block the broadcasting, we will create empty key states for all clients that are not connected
                for (auto& client_info : client_infos) {
                    if (not client_info.is_connected()) {
                        while (client_info.tetrion.next_frame() < min_num_frames_simulated) {
                            static constexpr auto key_state = KeyState{};
                            client_info.key_states.push_back(key_state);
                            // We simulate the frame here, because we won't receive any key states from the client.
                            // We ignore the return value because this client is not allowed to send any garbage since
                            // it is not connected anymore.
                            std::ignore = client_info.tetrion.simulate_next_frame(key_state);
                        }
                    }
                }
                auto const frame = min_num_frames_simulated;
                if (frame == 0) {
                    return num_clients_connected;
                }

                if (not last_min_num_frames_simulated.has_value() or frame != last_min_num_frames_simulated.value()) {
                    auto const broadcast_message = create_broadcast_message(client_infos, frame - 1);
                    for (auto& socket : self.m_client_sockets) {
                        if (socket.is_connected()) {
                            spdlog::info(
                                "sending broadcast message to socket with descriptor {}",
                                socket.os_socket_handle().value()
                            );
                            std::ignore = socket.send(broadcast_message);
                        }
                    }
                    last_min_num_frames_simulated = frame;
                }
                return num_clients_connected;
            });

        if (num_clients_connected == 0) {
            spdlog::info("stopping server");
            self.stop();
            break;
        }

        // todo: replace sleep with condition variable
        std::this_thread::sleep_for(100ms);
    }
}
