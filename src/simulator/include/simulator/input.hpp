#pragma once

#include <cstdint>

enum class Key {
    Left,
    Right,
    Down,
    Drop,
    RotateClockwise,
    RotateCounterClockwise,
    Hold,
};

enum class EventType {
    Pressed,
    Released,
};

struct Event {
    Key key;
    EventType type;
    std::uint64_t frame;

    [[nodiscard]] constexpr bool operator==(Event const&) const = default;
};
