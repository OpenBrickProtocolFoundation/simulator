#include "messages.hpp"

[[nodiscard]] std::unique_ptr<AbstractMessage> AbstractMessage::from_socket(c2k::ClientSocket& socket) {
    auto buffer = c2k::Extractor{};
    buffer << socket.receive(1).get();
    assert(buffer.size() == 1);
    auto const message_type = static_cast<MessageType>(buffer.try_extract<std::uint8_t>().value());

    assert(buffer.size() == 0);
    while (true) {
        buffer << socket.receive(8 - buffer.size()).get();
        if (buffer.size() >= 8) {
            break;
        }
    }
    assert(buffer.size() == 8);

    auto const size = static_cast<std::size_t>(buffer.try_extract<std::uint64_t>().value());
    assert(buffer.size() == 0);

    while (true) {
        buffer << socket.receive(size - buffer.size()).get();
        if (buffer.size() >= size) {
            break;
        }
    }
    assert(buffer.size() == size);

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
