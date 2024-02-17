#pragma once

#include <utility>

enum class Rotation {
    North = 0,
    East,
    South,
    West,
};

static_assert(std::to_underlying(Rotation::North) == 0, "other code relies on this 🙂🔫");
static_assert(std::to_underlying(Rotation::East) == 1, "other code relies on this 🙂🔫");
static_assert(std::to_underlying(Rotation::South) == 2, "other code relies on this 🙂🔫");
static_assert(std::to_underlying(Rotation::West) == 3, "other code relies on this 🙂🔫");
