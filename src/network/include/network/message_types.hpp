#pragma once

#include <cstdint>
#include <optional>

enum class MessageType : std::uint8_t {
    Heartbeat = 0,
    GridState,
    GameStart,
    StateBroadcast,
};

[[nodiscard]] inline std::optional<MessageType> convert_to_message_type(std::underlying_type_t<MessageType> const value) {
    switch (MessageType{ value }) {
        case MessageType::Heartbeat:
        case MessageType::GridState:
        case MessageType::GameStart:
        case MessageType::StateBroadcast:
            return MessageType{ value };
    }
    return std::nullopt;
}
