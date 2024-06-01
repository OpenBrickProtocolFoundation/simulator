#include "server.hpp"
#include "network/messages.hpp"
#include <algorithm>
#include <chrono>
#include <numeric>
#include <ranges>
#include <spdlog/spdlog.h>

void Server::process_client(std::stop_token const& stop_token, Server& self, std::size_t index) {
    using namespace std::chrono_literals;
    auto& socket = self.m_client_sockets.at(index);

    while (not stop_token.stop_requested() and socket.is_connected()) {
        auto message = std::unique_ptr<AbstractMessage>{};
        try {
            message = AbstractMessage::from_socket(socket);
        } catch (c2k::TimeoutError const& exception) {
            spdlog::error("timeout error while waiting for next client message: {}", exception.what());
            continue; // todo: require clients to send heartbeats even before the game starts
        } catch (c2k::ReadError const& exception) {
            spdlog::error("error while reading from socket: {}", exception.what());
            break;
        }
        if (message->type() == MessageType::Heartbeat) {
            spdlog::info("received heartbeat message");
            auto const& heartbeat_message = dynamic_cast<Heartbeat const&>(*message);
            // todo: check whether data is valid

            self.m_client_infos.apply([&heartbeat_message, index](std::vector<ClientInfo>& client_infos) {
                auto& client_info = client_infos.at(index);

                auto& tetrion = client_info.tetrion;
                for (auto const& event : heartbeat_message.events) {
                    auto const obpf_event = Event{ event.key, event.type, event.frame };
                    tetrion.enqueue_event(obpf_event);
                    client_info.event_buffer.push_back(obpf_event);
                    spdlog::info(
                            "enqueueing event: key {}, type {}, frame {}",
                            std::to_underlying(event.key),
                            std::to_underlying(event.type),
                            event.frame
                    );
                }

                auto& num_frames_simulated = client_info.num_frames_simulated;
                spdlog::info("simulating up until frame {}", heartbeat_message.frame);
                while (num_frames_simulated <= heartbeat_message.frame) {
                    tetrion.simulate_up_until(num_frames_simulated);
                    ++num_frames_simulated;
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
) { // clang-format on
    spdlog::info("creating broadcast message for frame {}", frame);
    auto message = c2k::MessageBuffer{};
    message << std::to_underlying(MessageType::EventBroadcast);
    auto const total_num_events = std::accumulate(
            client_infos.cbegin(),
            client_infos.cend(),
            std::size_t{ 0 },
            [](std::size_t const previous, ClientInfo const& client_info) {
                return previous + client_info.event_buffer.size();
            }
    );
    auto const payload_size = static_cast<std::uint16_t>(9 + client_infos.size() * 2 + total_num_events * 10);
    message << payload_size << frame << static_cast<std::uint8_t>(client_infos.size());
    spdlog::info("assembling broadcast message with data of {} clients", client_infos.size());
    for (auto& client : client_infos) {
        message << client.id << static_cast<std::uint8_t>(client.event_buffer.size());
        for (auto const& event : client.event_buffer) {
            message << static_cast<std::uint8_t>(event.key) << static_cast<std::uint8_t>(event.type) << event.frame;
        }
        client.event_buffer.clear();
    }
    static constexpr auto message_header_size = sizeof(MessageHeader::type) + sizeof(MessageHeader::payload_size);
    assert(payload_size + message_header_size == message.size());
    return message;
}

void Server::keep_broadcasting(std::stop_token const& stop_token, Server& self) {
    using namespace std::chrono_literals;
    auto last_min_num_frames_simulated = std::optional<std::uint64_t>{ std::nullopt };

    // wait for all clients to connect
    while (not stop_token.stop_requested()) {
        auto const num_connected_clients = self.m_client_infos.apply([](std::vector<ClientInfo> const& client_infos) {
            return client_infos.size();
        });
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
        auto const message = GameStart{ i, 180, self.m_seed };
        socket.send(message.serialize()).wait();
        ++i;
    }

    while (not stop_token.stop_requested()) {
        self.m_client_infos.apply([&self, &last_min_num_frames_simulated](std::vector<ClientInfo>& client_infos) {
            auto const min_num_frames_simulated = std::min_element(
                    client_infos.cbegin(),
                    client_infos.cend(),
                    [](ClientInfo const& lhs, ClientInfo const& rhs) {
                        return lhs.num_frames_simulated < rhs.num_frames_simulated;
                    }
            );
            if (min_num_frames_simulated->num_frames_simulated == 0) {
                return;
            }
            // clang-format off
            if (
                auto const frame = min_num_frames_simulated->num_frames_simulated;
                not last_min_num_frames_simulated.has_value() or frame != last_min_num_frames_simulated.value()
            ) { // clang-format on
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
