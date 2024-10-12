#include <magic_enum.hpp>
#include <memory>
#include <simulator/multiplayer_tetrion.hpp>

NullableUniquePointer<MultiplayerTetrion> MultiplayerTetrion::create(std::string const& server, std::uint16_t const port) {
    auto socket = c2k::Sockets::create_client(c2k::AddressFamily::Ipv4, server, port);
    auto message = std::unique_ptr<AbstractMessage>{};
    while (true) {
        try {
            message = AbstractMessage::from_socket(socket);
            break;
        } catch (c2k::TimeoutError const& exception) {
            spdlog::error("timeout error while waiting for game start message: {}", exception.what());
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

    auto const num_observers = game_start_message.num_players - 1;
    auto observers = std::vector<std::unique_ptr<ObserverTetrion>>{};
    observers.reserve(num_observers);
    auto observer_id = std::uint8_t{ 0 };
    for (auto i = std::size_t{ 0 }; i < num_observers; ++i) {
        if (observer_id == game_start_message.client_id) {
            ++observer_id;
        }
        observers.push_back(
            std::make_unique<ObserverTetrion>(game_start_message.random_seed, observer_id, ObserverTetrion::Key{})
        );
        ++observer_id;
    }

    return std::make_unique<MultiplayerTetrion>(
        std::move(socket),
        game_start_message.client_id,
        game_start_message.start_frame,
        game_start_message.random_seed,
        std::move(observers),
        Key{}
    );
}

void MultiplayerTetrion::simulate_next_frame(KeyState const key_state) {
    m_key_state_buffer.push_back(key_state);
    if (m_key_state_buffer.size() == heartbeat_interval) {
        send_heartbeat_message();
        m_key_state_buffer = {};
    }
    ObpfTetrion::simulate_next_frame(key_state);

    m_broadcast_queue.apply([this](std::deque<StateBroadcast>& queue) {
        while (not queue.empty()) {
            auto const message = std::move(queue.front());
            queue.pop_front();
            process_state_broadcast_message(message);
        }
    });
}

[[nodiscard]] std::vector<ObserverTetrion*> MultiplayerTetrion::get_observers() const {
    auto result = std::vector<ObserverTetrion*>{};
    result.reserve(m_observers.size());
    for (auto const& observer : m_observers) {
        result.push_back(observer.get());
    }
    return result;
}

void MultiplayerTetrion::send_heartbeat_message() {
    auto key_states = std::array<KeyState, heartbeat_interval>{};
    std::copy_n(m_key_state_buffer.cbegin(), heartbeat_interval, key_states.begin());
    auto const message = Heartbeat{ next_frame(), key_states };
    // don't wait blocking for the send to complete
    std::ignore = m_socket.send(message.serialize());
}

void MultiplayerTetrion::process_state_broadcast_message(StateBroadcast const& message) {
    for (auto const& client_states : message.states_per_client) {
        auto const& [id, states] = client_states;
        auto observer_it = std::ranges::find_if(m_observers, [id](auto const& observer) -> bool {
            return observer->client_id() == id;
        });
        if (observer_it == m_observers.end()) {
            if (id != m_client_id) {
                spdlog::error("received state broadcast for unknown client id {}", id);
            }
            continue;
        }
        spdlog::info("advancing observer {} to frame {}", id, message.frame);
        for (auto const& state : states) {
            (*observer_it)->process_key_state(state);
        }
        spdlog::info("observer frame after simulation: {}", (*observer_it)->next_frame());
    }
}

void MultiplayerTetrion::keep_receiving(
    std::stop_token const& stop_token,
    c2k::ClientSocket& socket,
    c2k::Synchronized<std::deque<StateBroadcast>>& queue
) {
    while (not stop_token.stop_requested()) {
        try {
            auto const message = AbstractMessage::from_socket(socket);
            switch (message->type()) {
                case MessageType::StateBroadcast: {
                    auto& state_broadcast_message = dynamic_cast<StateBroadcast&>(*message);
                    spdlog::info(
                        "received state broadcast message for frame {} ({} clients)",
                        state_broadcast_message.frame,
                        state_broadcast_message.states_per_client.size()
                    );
                    queue.apply([&state_broadcast_message](std::deque<StateBroadcast>& queue) {
                        queue.push_back(state_broadcast_message);
                    });
                    break;
                }
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
