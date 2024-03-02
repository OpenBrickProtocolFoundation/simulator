#include "network/constants.hpp"
#include "network/message_header.hpp"


#include <network/messages.hpp>

#include <cassert>
#include <chrono>
#include <limits>

static constexpr auto header_size = sizeof(std::underlying_type_t<MessageType>) + sizeof(MessageHeader::payload_size);

[[nodiscard]] Event Event::deserialize(c2k::Extractor& buffer) {
    auto const key = static_cast<ObpfKey>(buffer.try_extract<std::uint8_t>().value());
    auto const event_type = static_cast<ObpfEventType>(buffer.try_extract<std::uint8_t>().value());
    auto const event_frame = buffer.try_extract<std::uint64_t>().value();
    return Event{ key, event_type, event_frame };
}

static c2k::MessageBuffer& operator<<(c2k::MessageBuffer& buffer, Event const& event) {
    return buffer << static_cast<std::uint8_t>(event.key) << static_cast<std::uint8_t>(event.type) << event.frame;
}

[[nodiscard]] std::unique_ptr<AbstractMessage>
AbstractMessage::from_socket(c2k::ClientSocket& socket, std::chrono::steady_clock::duration const timeout) {
    using std::chrono::steady_clock;

    auto const end_time = steady_clock::now() + timeout;
    auto const remaining_time = [end_time] { return end_time - steady_clock::now(); };

    auto buffer = c2k::Extractor{};
    buffer << socket.receive_exact(1, remaining_time()).get();
    assert(buffer.size() == 1);
    auto const message_type = static_cast<MessageType>(buffer.try_extract<std::uint8_t>().value());

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
        }
        return max_payload_size;
    }();

    assert(buffer.size() == 0);

    buffer << socket.receive_exact(sizeof(MessageHeader::payload_size), remaining_time()).get();
    assert(buffer.size() == sizeof(MessageHeader::payload_size));
    auto const payload_size =
            static_cast<std::size_t>(buffer.try_extract<decltype(MessageHeader::payload_size)>().value());
    assert(buffer.size() == 0);

    if (payload_size == 0) {
        throw MessageSerializationError{ std::format(
                "message payload size 0 is invalid",
                payload_size,
                std::to_underlying(message_type),
                message_max_payload_size
        ) };
    }

    if (payload_size > message_max_payload_size) {
        throw MessageSerializationError{ std::format(
                "message payload size {} is too big for message type {} (maximum is {})",
                payload_size,
                std::to_underlying(message_type),
                message_max_payload_size
        ) };
    }

    buffer << socket.receive_exact(payload_size, remaining_time()).get();
    assert(buffer.size() == payload_size);

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
    throw std::runtime_error{ std::format("{} is not a known message type", std::to_underlying(message_type)) };
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
    assert(buffer.data.size() == payload_size() + header_size);
    return buffer;
}

[[nodiscard]] Heartbeat Heartbeat::deserialize(c2k::Extractor& buffer) {
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
    assert(buffer.data.size() == payload_size() + header_size);
    return buffer;
}

[[nodiscard]] GridState GridState::deserialize(c2k::Extractor& buffer) {
    auto const frame = buffer.try_extract<std::uint64_t>().value();
    auto grid_contents = decltype(GridState::grid_contents){};
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
    assert(buffer.data.size() == payload_size() + header_size);
    return buffer;
}

[[nodiscard]] GameStart GameStart::deserialize(c2k::Extractor& buffer) {
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
    assert(buffer.data.size() == payload_size() + header_size);
    return buffer;
}

[[nodiscard]] EventBroadcast EventBroadcast::deserialize(c2k::Extractor& buffer) {
    auto const frame = buffer.try_extract<std::uint64_t>().value();
    auto const num_clients = static_cast<std::size_t>(buffer.try_extract<std::uint8_t>().value());
    auto events_per_client = decltype(EventBroadcast::events_per_client){};
    events_per_client.reserve(num_clients);
    for (auto i = std::size_t{ 0 }; i < num_clients; ++i) {
        auto events = ClientEvents{};
        events.client_id = buffer.try_extract<std::uint8_t>().value();
        auto const num_events = static_cast<std::size_t>(buffer.try_extract<std::uint8_t>().value());
        events.events.reserve(num_events);
        for (auto j = std::size_t{ 0 }; j < num_events; ++j) {
            events.events.push_back(Event::deserialize(buffer));
        }
        events_per_client.push_back(std::move(events));
    }
    assert(buffer.size() == 0);
    return EventBroadcast{ frame, std::move(events_per_client) };
}
