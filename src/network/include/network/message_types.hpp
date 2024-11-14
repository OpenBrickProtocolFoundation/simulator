#pragma once

#include <cstdint>

enum class MessageType : std::uint8_t {
    Connect,
    Heartbeat,
    GridState,
    GameStart,
    StateBroadcast,
    ClientDisconnected,
};
