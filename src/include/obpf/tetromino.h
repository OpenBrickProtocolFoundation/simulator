#pragma once

#include "tetromino_type.h"
#include "vec2.h"

struct ObpfTetromino {
    ObpfVec2 mino_positions[4];
    ObpfTetrominoType type;
};
