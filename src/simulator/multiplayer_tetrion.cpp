#include <algorithm>
#include <magic_enum.hpp>
#include <memory>
#include <ranges>
#include <simulator/multiplayer_tetrion.hpp>

NullableUniquePointer<MultiplayerTetrion> MultiplayerTetrion::create(
    std::string const& server,
    std::uint16_t const port,
    std::string player_name
) {
    auto socket = c2k::Sockets::create_client(c2k::AddressFamily::Ipv4, server, port);
    auto message = std::unique_ptr<AbstractMessage>{};

    // Identify this client...
    socket.send(Connect{ player_name }.serialize()).wait();

    // Wait for the GameStart message coming from the server...
    while (true) {
        try {
            message = AbstractMessage::from_socket(socket);
            break;
        } catch (c2k::TimeoutError const&) {
            spdlog::info("waiting for the game to start...");
        } catch (c2k::ReadError const& exception) {
            spdlog::error("error while reading from socket: {}", exception.what());
            return nullptr;
        }
    }
    if (message->type() != MessageType::GameStart) {
        spdlog::error(
            "expected game start message, but received message of type {}",
            magic_enum::enum_name(message->type())
        );
        return nullptr;
    }
    spdlog::info("received game start message");
    auto const& game_start_message = dynamic_cast<GameStart const&>(*message);

    auto const num_observers = static_cast<usize>(game_start_message.num_players() - 1);
    auto observers = std::vector<std::unique_ptr<ObserverTetrion>>{};
    observers.reserve(num_observers);
    auto observer_id = u8{ 0 };
    for (auto i = usize{ 0 }; i < num_observers; ++i) {
        if (observer_id == game_start_message.client_id) {
            ++observer_id;
        }

        auto const find_iterator =
            std::ranges::find_if(game_start_message.client_identities, [observer_id](auto const& identity) {
                return identity.client_id == observer_id;
            });

        using namespace std::string_literals;
        // clang-format off
        auto observer_name = (
            find_iterator == game_start_message.client_identities.cend()
            ? "<unknown observer name>"s
            : find_iterator->player_name
        );
        // clang-format on

        observers.push_back(std::make_unique<ObserverTetrion>(
            game_start_message.random_seed,
            game_start_message.start_frame,
            observer_id,
            std::move(observer_name),
            ObserverTetrion::Key{}
        ));
        ++observer_id;
    }

    auto const find_iterator =
        std::ranges::find_if(game_start_message.client_identities, [&game_start_message](auto const& identity) {
            return identity.client_id == game_start_message.client_id;
        });

    // clang-format off
    auto this_player_name = (
        find_iterator == game_start_message.client_identities.cend()
        ? player_name
        : find_iterator->player_name
    );
    // clang-format on

    return std::make_unique<MultiplayerTetrion>(
        std::move(socket),
        game_start_message.client_id,
        game_start_message.start_frame,
        game_start_message.random_seed,
        std::move(observers),
        std::move(this_player_name),
        Key{}
    );
}

[[nodiscard]] std::optional<GarbageSendEvent> MultiplayerTetrion::simulate_next_frame(KeyState const key_state) {
    m_key_state_buffer.push_back(key_state);
    if (m_key_state_buffer.size() == heartbeat_interval) {
        send_heartbeat_message();
        m_key_state_buffer = {};
    }
    auto const outgoing_garbage = ObpfTetrion::simulate_next_frame(key_state);
    if (outgoing_garbage.has_value()) {
        // clang-format off
        m_outgoing_garbage_queue.apply(
            [outgoing_garbage = outgoing_garbage.value()](std::deque<GarbageSendEvent>& queue) {
                queue.push_back(outgoing_garbage);
            }
        );
        // clang-format on
    }

    m_message_queue.apply([this](std::deque<std::unique_ptr<AbstractMessage>>& queue) {
        while (not queue.empty()) {
            auto const message = std::move(queue.front());
            queue.pop_front();
            switch (message->type()) {
                case MessageType::StateBroadcast:
                    process_state_broadcast_message(dynamic_cast<StateBroadcast const&>(*message));
                    break;
                case MessageType::ClientDisconnected:
                    on_client_disconnected(dynamic_cast<ClientDisconnected const&>(*message).client_id);
                    break;
                default:
                    spdlog::error("cannot handle message of type {}", magic_enum::enum_name(message->type()));
                    break;
            }
        }
    });
    return outgoing_garbage;
}

[[nodiscard]] std::vector<ObserverTetrion*> MultiplayerTetrion::get_observers() const {
    auto result = std::vector<ObserverTetrion*>{};
    result.reserve(m_observers.size());
    for (auto const& observer : m_observers) {
        result.push_back(observer.get());
    }
    return result;
}

void MultiplayerTetrion::on_client_disconnected(u8 const client_id) {
    auto const find_iterator = std::ranges::find_if(m_observers, [client_id](auto const& observer) -> bool {
        return observer->id() == client_id;
    });
    if (find_iterator == m_observers.end()) {
        spdlog::error("client {} disconnected, but no observer found", client_id);
        return;
    }
    (*find_iterator)->m_is_connected = false;
    // todo: set the observer to be game over
}

void MultiplayerTetrion::send_heartbeat_message() {
    auto key_states = std::array<KeyState, heartbeat_interval>{};
    std::copy_n(m_key_state_buffer.cbegin(), heartbeat_interval, key_states.begin());
    auto const message = Heartbeat{ next_frame(), key_states };
    // don't wait blocking for the send to complete
    std::ignore = m_socket.send(message.serialize());
}

void MultiplayerTetrion::process_state_broadcast_message(StateBroadcast const& message) {
    auto tetrions = std::vector<ObpfTetrion*>{};
    tetrions.reserve(m_observers.size() + 1);
    tetrions.push_back(this);
    for (auto const& observer : m_observers) {
        tetrions.push_back(observer.get());
    }

    for (auto i = usize{ 0 }; i < std::tuple_size_v<decltype(StateBroadcast::ClientStates::states)>; ++i) {
        if (m_observers.empty()) {
            break;
        }
        assert(
            std::ranges::all_of(
                m_observers,
                [&](auto const& observer) { return observer->next_frame() == m_observers.front()->next_frame(); }
            )
            and "not all observers are synchronized"
        );
        auto const observers_frame = m_observers.front()->next_frame();

        auto garbage_send_events = std::unordered_map<u8, GarbageSendEvent>{};
        for (auto const& client_states : message.states_per_client) {
            auto const& [id, states] = client_states;
            auto observer_it =
                std::ranges::find_if(m_observers, [id](auto const& observer) -> bool { return observer->id() == id; });
            if (observer_it == m_observers.end()) {
                continue;
            }
            if (auto const garbage_send_event = (*observer_it)->process_key_state(states.at(i));
                garbage_send_event.has_value()) {
                garbage_send_events.emplace(id, garbage_send_event.value());
            }
        }

        // apply garbage coming from ourselves
        while (true) {
            auto const garbage = m_outgoing_garbage_queue.apply(
                [observers_frame](std::deque<GarbageSendEvent>& queue) -> std::optional<GarbageSendEvent> {
                    if (queue.empty()) {
                        return std::nullopt;
                    }
                    auto const garbage = queue.front();
                    if (garbage.frame < observers_frame) {
                        return std::nullopt;
                    }
                    queue.pop_front();
                    return garbage;
                }
            );
            if (not garbage.has_value()) {
                break;
            }
            auto target_tetrion = determine_garbage_target(tetrions, id(), garbage.value().frame);
            if (not target_tetrion.has_value()) {
                continue;
            }
            target_tetrion.value().receive_garbage(garbage.value());
        }

        // apply garbage between observers
        for (auto const [sender_client_id, garbage_send_event] : garbage_send_events) {
            auto target_tetrion = determine_garbage_target(tetrions, sender_client_id, garbage_send_event.frame);
            if (not target_tetrion.has_value()) {
                continue;
            }
            if (target_tetrion.value().id() == id()) {
                receive_garbage(garbage_send_event);
                continue;
            }
            target_tetrion.value().receive_garbage(garbage_send_event);
        }
    }
}

void MultiplayerTetrion::keep_receiving(
    std::stop_token const& stop_token,
    c2k::ClientSocket& socket,
    c2k::Synchronized<std::deque<std::unique_ptr<AbstractMessage>>>& queue
) {
    while (not stop_token.stop_requested()) {
        try {
            auto message = AbstractMessage::from_socket(socket);
            switch (message->type()) {
                case MessageType::StateBroadcast:
                case MessageType::ClientDisconnected:
                    spdlog::info("queueing message of type {}", magic_enum::enum_name(message->type()));
                    // clang-format off
                    queue.apply(
                        [message = std::move(message)](std::deque<std::unique_ptr<AbstractMessage>>& queue) mutable {
                            queue.push_back(std::move(message));
                        }
                    );
                    // clang-format on
                    break;
                default:
                    spdlog::warn("received message of unexpected type: {}", magic_enum::enum_name(message->type()));
                    break;
            }
        } catch (c2k::TimeoutError const&) {
            // swallow exception because this is expected
        } catch (c2k::ReadError const& exception) {
            spdlog::error("error while reading from socket: {}", exception.what());
            break;
        }
    }
}
