#include <cassert>
#include <chrono>
#include <limits>
#include <network/messages.hpp>
#include <unordered_set>
#include "network/constants.hpp"
#include "network/message_header.hpp"

[[maybe_unused]] static constexpr auto header_size =
    sizeof(std::underlying_type_t<MessageType>) + sizeof(MessageHeader::payload_size);

// clang-format off
[[nodiscard]] std::unique_ptr<AbstractMessage> AbstractMessage::from_socket(
    c2k::ClientSocket& socket,
    std::chrono::steady_clock::duration const timeout
) {  // clang-format on
    using std::chrono::steady_clock;

    auto const end_time = steady_clock::now() + timeout;
    auto const remaining_time = [end_time] {
        return end_time - steady_clock::now();
    };

    auto const message_type = static_cast<MessageType>(socket.receive<std::uint8_t>().get());

    auto const message_max_payload_size = [message_type] {
        switch (message_type) {
            case MessageType::Heartbeat:
                return Heartbeat::max_payload_size();
            case MessageType::GridState:
                return GridState::max_payload_size();
            case MessageType::GameStart:
                return GameStart::max_payload_size();
            case MessageType::StateBroadcast:
                return StateBroadcast::max_payload_size();
            case MessageType::ClientDisconnected:
                return ClientDisconnected::max_payload_size();
                // todo: add case for custom messages
        }
        throw MessageDeserializationError{ std::format("{} is an unknown message type", static_cast<int>(message_type)) };
    }();

    auto buffer = c2k::MessageBuffer{};
    buffer << socket.receive_exact(sizeof(MessageHeader::payload_size), remaining_time()).get();
    assert(buffer.size() == sizeof(MessageHeader::payload_size));
    auto const payload_size =
        static_cast<std::size_t>(buffer.try_extract<decltype(MessageHeader::payload_size)>().value());
    assert(buffer.size() == 0);

    if (payload_size == 0) {
        throw MessageDeserializationError{ std::format(
            "message payload size 0 is invalid",
            payload_size,
            std::to_underlying(message_type),
            message_max_payload_size
        ) };
    }

    if (payload_size > message_max_payload_size) {
        throw MessageDeserializationError{ std::format(
            "message payload size {} is too big for message type {} (maximum is {})",
            payload_size,
            std::to_underlying(message_type),
            message_max_payload_size
        ) };
    }

    buffer << socket.receive_exact(payload_size, remaining_time()).get();
    assert(buffer.size() == payload_size);

    try {
        switch (message_type) {
            case MessageType::Heartbeat:
                return std::make_unique<Heartbeat>(Heartbeat::deserialize(buffer));
            case MessageType::GridState:
                return std::make_unique<GridState>(GridState::deserialize(buffer));
            case MessageType::GameStart:
                return std::make_unique<GameStart>(GameStart::deserialize(buffer));
            case MessageType::StateBroadcast:
                return std::make_unique<StateBroadcast>(StateBroadcast::deserialize(buffer));
            case MessageType::ClientDisconnected:
                return std::make_unique<ClientDisconnected>(ClientDisconnected::deserialize(buffer));
        }
    } catch (MessageInstantiationError const& exception) {
        throw MessageDeserializationError{ std::format("failed to deserialize message: {}", exception.what()) };
    }
    std::unreachable();
}

[[nodiscard]] MessageType Heartbeat::type() const {
    return MessageType::Heartbeat;
}

[[nodiscard]] c2k::MessageBuffer Heartbeat::serialize() const {
    auto buffer = c2k::MessageBuffer{};
    buffer << static_cast<std::uint8_t>(MessageType::Heartbeat) << payload_size() << frame;
    for (auto const& key_state : key_states) {
        buffer << key_state.get_bitmask();
    }
    assert(buffer.size() == payload_size() + header_size);
    return buffer;
}

[[nodiscard]] Heartbeat Heartbeat::deserialize(c2k::MessageBuffer& buffer) {
    auto const frame = buffer.try_extract<std::uint64_t>().value();

    auto key_states = decltype(Heartbeat::key_states){};
    for (auto& key_state : key_states) {
        auto const state = KeyState::from_bitmask(buffer.try_extract<std::uint8_t>().value());
        if (not state.has_value()) {
            throw MessageDeserializationError{ "failed to deserialize KeyState from bitmask" };
        }
        key_state = state.value();
    }

    return Heartbeat{ frame, key_states };
}

[[nodiscard]] MessageType GridState::type() const {
    return MessageType::GridState;
}

[[nodiscard]] decltype(MessageHeader::payload_size) GridState::payload_size() const {
    return calculate_payload_size();
}

[[nodiscard]] c2k::MessageBuffer GridState::serialize() const {
    auto buffer = c2k::MessageBuffer{};
    // clang-format off
        buffer << static_cast<std::uint8_t>(MessageType::GridState)
               << payload_size()
               << frame;
    // clang-format on
    for (auto const tetromino_type : grid_contents) {
        buffer << static_cast<std::uint8_t>(tetromino_type);
    }
    assert(buffer.size() == payload_size() + header_size);
    return buffer;
}

[[nodiscard]] GridState GridState::deserialize(c2k::MessageBuffer& buffer) {
    auto const frame = buffer.try_extract<std::uint64_t>().value();
    auto grid_contents = decltype(GridState::grid_contents){};
    if (buffer.size() < grid_contents.size()) {
        throw MessageDeserializationError{ std::format(
            "too few bytes to deserialize GridState message ({} needed, {} received)",
            grid_contents.size(),
            buffer.size()
        ) };
    }
    for (auto& tetromino : grid_contents) {
        tetromino = static_cast<TetrominoType>(buffer.try_extract<std::uint8_t>().value());
    }
    assert(buffer.size() == 0);
    return GridState{ frame, grid_contents };
}

[[nodiscard]] MessageType GameStart::type() const {
    return MessageType::GameStart;
}

[[nodiscard]] decltype(MessageHeader::payload_size) GameStart::payload_size() const {
    return calculate_payload_size();
}

[[nodiscard]] c2k::MessageBuffer GameStart::serialize() const {
    auto buffer = c2k::MessageBuffer{};
    // clang-format off
        buffer << static_cast<std::uint8_t>(MessageType::GameStart)
               << payload_size()
               << client_id
               << start_frame
               << random_seed
               << num_players;
    // clang-format on
    assert(buffer.size() == payload_size() + header_size);
    return buffer;
}

[[nodiscard]] GameStart GameStart::deserialize(c2k::MessageBuffer& buffer) {
    // clang-format off
    static constexpr auto required_num_bytes = c2k::detail::summed_sizeof<
        decltype(client_id),
        decltype(start_frame),
        decltype(random_seed),
        decltype(num_players)
    >();
    // clang-format on
    if (buffer.size() < required_num_bytes) {
        throw MessageDeserializationError{ std::format(
            "too few bytes to deserialize GameStart message ({} needed, {} received)",
            required_num_bytes,
            buffer.size()
        ) };
    }
    // clang-format off
    auto const [
        client_id,
        start_frame,
        random_seed,
        num_players
    ] = buffer.try_extract<
            decltype(GameStart::client_id),
            decltype(GameStart::start_frame),
            decltype(GameStart::random_seed),
            decltype(GameStart::num_players)
        >()
        .value();
    // clang-format on
    assert(buffer.size() == 0);
    return GameStart{ client_id, start_frame, random_seed, num_players };
}

StateBroadcast::StateBroadcast(std::uint64_t const frame, std::vector<ClientStates> states_per_client)
    : frame{ frame }, states_per_client{ std::move(states_per_client) } {
    assert((frame + 1) % heartbeat_interval == 0);
    if (this->states_per_client.size() > std::numeric_limits<std::uint8_t>::max()) {
        throw MessageInstantiationError{ fmt::format(
            "cannot instantiate EventBroadcast message with {} clients ({} is maximum)",
            this->states_per_client.size(),
            std::numeric_limits<std::uint8_t>::max()
        ) };
    }

    auto contained_client_ids = std::unordered_set<decltype(ClientStates::client_id)>{};

    for (auto const& [client_id, states] : this->states_per_client) {
        auto const [_, inserted] = contained_client_ids.insert(client_id);
        if (not inserted) {
            throw MessageInstantiationError{
                std::format("duplicate client id {} while trying to instantiate EventBroadcast message", client_id)
            };
        }
    }
}

[[nodiscard]] MessageType StateBroadcast::type() const {
    return MessageType::StateBroadcast;
}

[[nodiscard]] decltype(MessageHeader::payload_size) StateBroadcast::payload_size() const {
    return calculate_payload_size(states_per_client.size());
}

[[nodiscard]] c2k::MessageBuffer StateBroadcast::serialize() const {
    assert(states_per_client.size() <= std::numeric_limits<std::uint8_t>::max());
    auto buffer = c2k::MessageBuffer{};
    // clang-format off
    buffer << static_cast<std::uint8_t>(MessageType::StateBroadcast)
           << payload_size()
           << frame
           << static_cast<std::uint8_t>(states_per_client.size()); // num clients
    // clang-format on
    for (auto const [client_id, states] : states_per_client) {
        buffer << client_id;
        for (auto const state : states) {
            buffer << state.get_bitmask();
        }
    }
    assert(buffer.size() == payload_size() + header_size);
    return buffer;
}

[[nodiscard]] StateBroadcast StateBroadcast::deserialize(c2k::MessageBuffer& buffer) {
    using Frame = decltype(frame);
    using NumClients = std::uint8_t;
    if (buffer.size() < sizeof(Frame) + sizeof(NumClients)) {
        throw MessageDeserializationError{ "too few bytes to deserialize StateBroadcast message" };
    }
    auto const frame = buffer.try_extract<Frame>().value();
    auto const num_clients = static_cast<std::size_t>(buffer.try_extract<NumClients>().value());

    auto states_per_client = decltype(StateBroadcast::states_per_client){};
    states_per_client.reserve(num_clients);

    for (auto i = std::size_t{ 0 }; i < num_clients; ++i) {
        auto const client_id = buffer.try_extract<std::uint8_t>().value();
        auto states = std::array<KeyState, heartbeat_interval>{};
        for (auto& state : states) {
            auto const bitmask = buffer.try_extract<std::uint8_t>().value();
            auto const key_state = KeyState::from_bitmask(bitmask);
            if (not key_state.has_value()) {
                throw MessageDeserializationError{ "failed to deserialize KeyState from bitmask" };
            }
            state = key_state.value();
        }
        states_per_client.emplace_back(client_id, states);
    }

    if (buffer.size() > 0) {
        throw MessageDeserializationError{ "excess bytes while deserializing EventBroadcast message" };
    }
    return StateBroadcast{ frame, std::move(states_per_client) };
}

[[nodiscard]] MessageType ClientDisconnected::type() const {
    return MessageType::ClientDisconnected;
}

decltype(MessageHeader::payload_size) ClientDisconnected::payload_size() const {
    return max_payload_size();
}

[[nodiscard]] c2k::MessageBuffer ClientDisconnected::serialize() const {
    auto buffer = c2k::MessageBuffer{};
    buffer << static_cast<u8>(type()) << payload_size() << client_id;
    return buffer;
}

[[nodiscard]] ClientDisconnected ClientDisconnected::deserialize(c2k::MessageBuffer& buffer) {
    if (buffer.size() < max_payload_size()) {
        throw MessageDeserializationError{ "too few bytes to deserialize ClientDisconnected message" };
    }
    auto const client_id = buffer.try_extract<u8>().value();
    if (buffer.size() > 0) {
        throw MessageDeserializationError{ "excess bytes while deserializing ClientDisconnected message" };
    }
    return ClientDisconnected{ client_id };
}

bool ClientDisconnected::equals(AbstractMessage const& other) const {
    auto const other_client_disconnected = dynamic_cast<ClientDisconnected const*>(&other);
    return other_client_disconnected != nullptr and client_id == other_client_disconnected->client_id;
}
