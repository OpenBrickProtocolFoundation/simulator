#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "tetromino_type.h"
#include "vec2.h"

struct ObpfTetromino {
    ObpfVec2 mino_positions[4];
    ObpfTetrominoType type;
};

#ifdef __cplusplus
}
#endif
