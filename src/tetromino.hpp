#pragma once
#include "rotation.hpp"
#include "tetromino_type.hpp"
#include "vec2.hpp"
#include <obpf/tetromino.h>

struct Tetromino;

[[nodiscard]] std::array<Vec2, 4> get_mino_positions(Tetromino const& tetromino);

struct Tetromino final {
    Vec2 position;
    Rotation rotation;
    TetrominoType type;

    Tetromino(Vec2 const position_, Rotation const rotation_, TetrominoType const type_)
        : position{ position_ },
          rotation{ rotation_ },
          type{ type_ } { }

    [[nodiscard]] explicit operator ObpfTetromino() const {
        auto const mino_positions = get_mino_positions(*this);
        return ObpfTetromino{
            .mino_positions = { static_cast<ObpfVec2>(mino_positions.at(0)),
                               static_cast<ObpfVec2>(mino_positions.at(1)),
                               static_cast<ObpfVec2>(mino_positions.at(2)),
                               static_cast<ObpfVec2>(mino_positions.at(3)) },
            .type = static_cast<ObpfTetrominoType>(type),
        };
    }
};
