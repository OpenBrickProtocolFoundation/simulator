#include "network/constants.hpp"
#include "network/message_header.hpp"
#include <cassert>
#include <chrono>
#include <limits>
#include <network/messages.hpp>

static constexpr auto header_size = sizeof(std::underlying_type_t<MessageType>) + sizeof(MessageHeader::payload_size);

[[nodiscard]] Event Event::deserialize(c2k::MessageBuffer& buffer) {
    if (auto const extracted = buffer.try_extract<std::uint8_t, std::uint8_t, std::uint64_t>()) {
        auto const [key, event_type, event_frame] = extracted.value();
        return Event{ static_cast<ObpfKey>(key), static_cast<ObpfEventType>(event_type), event_frame };
    }
    throw EventDeserializationError{ "too few bytes to deserialize event" };
}

static c2k::MessageBuffer& operator<<(c2k::MessageBuffer& buffer, Event const& event) {
    return buffer << static_cast<std::uint8_t>(event.key) << static_cast<std::uint8_t>(event.type) << event.frame;
}

[[nodiscard]] std::unique_ptr<AbstractMessage>
AbstractMessage::from_socket(c2k::ClientSocket& socket, std::chrono::steady_clock::duration const timeout) {
    using std::chrono::steady_clock;

    auto const end_time = steady_clock::now() + timeout;
    auto const remaining_time = [end_time] { return end_time - steady_clock::now(); };

    auto const message_type = static_cast<MessageType>(socket.receive<std::uint8_t>().get());

    auto const message_max_payload_size = [message_type] {
        switch (message_type) {
            case MessageType::Heartbeat:
                return static_cast<std::uint16_t>(Heartbeat::max_payload_size());
            case MessageType::GridState:
                return static_cast<std::uint16_t>(GridState::max_payload_size());
            case MessageType::GameStart:
                return static_cast<std::uint16_t>(GameStart::max_payload_size());
            case MessageType::EventBroadcast:
                return static_cast<std::uint16_t>(EventBroadcast::max_payload_size());
                // todo: add case for custom messages
        }
        throw MessageDeserializationError{
            std::format("{} is an unknown message type", static_cast<int>(message_type))
        };
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
            case MessageType::EventBroadcast:
                return std::make_unique<EventBroadcast>(EventBroadcast::deserialize(buffer));
        }
    } catch (MessageInstantiationError const& exception) {
        throw MessageDeserializationError{ std::format("failed to deserialize message: {}", exception.what()) };
    }
    std::unreachable();
}

[[nodiscard]] MessageType Heartbeat::type() const {
    return MessageType::Heartbeat;
}

[[nodiscard]] decltype(MessageHeader::payload_size) Heartbeat::payload_size() const {
    return calculate_payload_size(events.size());
}

[[nodiscard]] c2k::MessageBuffer Heartbeat::serialize() const {
    assert(events.size() <= heartbeat_interval);
    auto buffer = c2k::MessageBuffer{};
    // clang-format off
        buffer << static_cast<std::uint8_t>(MessageType::Heartbeat)
               << payload_size()
               << frame
               << static_cast<std::uint8_t>(events.size());
    // clang-format on
    for (auto const& event : events) {
        buffer << event;
    }
    assert(buffer.size() == payload_size() + header_size);
    return buffer;
}

[[nodiscard]] Heartbeat Heartbeat::deserialize(c2k::MessageBuffer& buffer) {
    auto const frame = buffer.try_extract<std::uint64_t>().value();
    auto const num_events = static_cast<std::size_t>(buffer.try_extract<std::uint8_t>().value());
    auto events = std::vector<Event>{};
    events.reserve(num_events);
    for (auto i = std::size_t{ 0 }; i < num_events; ++i) {
        events.push_back(Event::deserialize(buffer));
    }
    assert(buffer.size() == 0);
    return Heartbeat{ frame, std::move(events) };
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
        tetromino = static_cast<ObpfTetrominoType>(buffer.try_extract<std::uint8_t>().value());
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
               << random_seed;
    // clang-format on
    assert(buffer.size() == payload_size() + header_size);
    return buffer;
}

[[nodiscard]] GameStart GameStart::deserialize(c2k::MessageBuffer& buffer) {
    static constexpr auto required_num_bytes = c2k::detail::summed_sizeof<std::uint8_t, std::uint64_t, std::uint64_t>();
    if (buffer.size() < required_num_bytes) {
        throw MessageDeserializationError{ std::format(
                "too few bytes to deserialize GameStart message ({} needed, {} received)",
                required_num_bytes,
                buffer.size()
        ) };
    }
    auto const [client_id, start_frame, random_seed] =
            buffer.try_extract<std::uint8_t, std::uint64_t, std::uint64_t>().value();
    assert(buffer.size() == 0);
    return GameStart{ client_id, start_frame, random_seed };
}

[[nodiscard]] MessageType EventBroadcast::type() const {
    return MessageType::EventBroadcast;
}

[[nodiscard]] decltype(MessageHeader::payload_size) EventBroadcast::payload_size() const {
    auto sizes = std::vector<std::size_t>{};
    sizes.reserve(events_per_client.size());
    for (auto const& [frame, events] : events_per_client) {
        sizes.push_back(events.size());
    }
    return calculate_payload_size(sizes);
}

[[nodiscard]] c2k::MessageBuffer EventBroadcast::serialize() const {
    assert(events_per_client.size() <= std::numeric_limits<std::uint8_t>::max());
    auto buffer = c2k::MessageBuffer{};
    // clang-format off
        buffer << static_cast<std::uint8_t>(MessageType::EventBroadcast)
               << payload_size()
               << frame
               << static_cast<std::uint8_t>(events_per_client.size()); // num clients
    // clang-format on
    for (auto const& events : events_per_client) {
        assert(events.events.size() <= heartbeat_interval);
        buffer << events.client_id << static_cast<std::uint8_t>(events.events.size());
        for (auto const& event : events.events) {
            buffer << event;
        }
    }
    assert(buffer.size() == payload_size() + header_size);
    return buffer;
}

[[nodiscard]] EventBroadcast EventBroadcast::deserialize(c2k::MessageBuffer& buffer) {
    using Frame = std::uint64_t;
    using NumClients = std::uint8_t;
    if (buffer.size() < sizeof(Frame) + sizeof(NumClients)) {
        throw MessageDeserializationError{ "too few bytes to deserialize EventBroadcast message" };
    }
    auto const frame = buffer.try_extract<Frame>().value();
    auto const num_clients = static_cast<std::size_t>(buffer.try_extract<NumClients>().value());
    auto events_per_client = decltype(EventBroadcast::events_per_client){};
    events_per_client.reserve(num_clients);
    for (auto i = std::size_t{ 0 }; i < num_clients; ++i) {
        auto events = ClientEvents{};
        if (auto const extracted = buffer.try_extract<std::uint8_t, std::uint8_t>()) {
            auto const [client_id, num_events] = extracted.value();
            events.client_id = client_id;
            events.events.reserve(static_cast<std::size_t>(num_events));
            for (auto j = std::size_t{ 0 }; j < num_events; ++j) {
                try {
                    events.events.push_back(Event::deserialize(buffer));
                } catch (EventDeserializationError const& exception) {
                    throw MessageDeserializationError{
                        std::format("failed to deserialize event inside EventBroadcast message: {}", exception.what())
                    };
                }
            }
            events_per_client.push_back(std::move(events));
        } else {
            throw MessageDeserializationError{ "too few bytes to deserialize EventBroadcast message" };
        }
    }
    if (buffer.size() > 0) {
        throw MessageDeserializationError{ "excess bytes while deserializing EventBroadcast message" };
    }
    return EventBroadcast{ frame, std::move(events_per_client) };
}
