#pragma once

#include "rotation.hpp"
#include "tetromino_type.hpp"
#include "vec2.hpp"


struct Tetromino final {
    Vec2 position;
    Rotation rotation;
    TetrominoType type;

    Tetromino(Vec2 const position_, Rotation const rotation_, TetrominoType const type_)
        : position{ position_ },
          rotation{ rotation_ },
          type{ type_ } { }
};

[[nodiscard]] std::array<Vec2, 4> get_mino_positions(Tetromino const& tetromino);
