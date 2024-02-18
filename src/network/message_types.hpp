#pragma once

#include <cstdint>

enum class MessageType : std::uint8_t {
    Heartbeat = 0,
    GridState,
    GameStart,
    EventBroadcast,
};
