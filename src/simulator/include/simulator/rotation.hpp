#pragma once

#include <utility>

enum class Rotation {
    North = 0,
    East,
    South,
    West,
    LastRotation = West,
};

static_assert(std::to_underlying(Rotation::North) == 0, "other code relies on this 🙂🔫");
static_assert(std::to_underlying(Rotation::East) == 1, "other code relies on this 🙂🔫");
static_assert(std::to_underlying(Rotation::South) == 2, "other code relies on this 🙂🔫");
static_assert(std::to_underlying(Rotation::West) == 3, "other code relies on this 🙂🔫");

inline Rotation& operator++(Rotation& rotation) {
    rotation = static_cast<Rotation>((static_cast<int>(rotation) + 1) % (static_cast<int>(Rotation::LastRotation) + 1));
    return rotation;
}

inline Rotation& operator--(Rotation& rotation) {
    rotation = static_cast<Rotation>(
            (static_cast<int>(rotation) + static_cast<int>(Rotation::LastRotation))
            % (static_cast<int>(Rotation::LastRotation) + 1)
    );
    return rotation;
}

[[nodiscard]] inline Rotation operator+(Rotation rotation, int const offset) {
    if (offset == 0) {
        return rotation;
    }

    if (offset > 0) {
        for (int i = 0; i < offset; ++i) {
            ++rotation;
        }
        return rotation;
    }

    for (int i = 0; i < -offset; ++i) {
        --rotation;
    }
    return rotation;
}

[[nodiscard]] inline Rotation operator-(Rotation const rotation, int const offset) {
    return rotation + (-offset);
}
