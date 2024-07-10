#pragma once

#include <array>
#include "tetromino.hpp"
#include "vec2.hpp"

[[nodiscard]] std::array<Vec2, 5> const& get_wall_kick_table(
    TetrominoType tetromino_type,
    Rotation from_rotation,
    Rotation to_rotation
);
