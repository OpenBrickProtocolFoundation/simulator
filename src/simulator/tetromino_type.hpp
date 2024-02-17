#pragma once

#include <obpf/tetromino_type.h>
#include <type_traits>

enum class TetrominoType : std::underlying_type_t<ObpfTetrominoType> {
    Empty = OBPF_TETROMINO_TYPE_EMPTY,
    I = OBPF_TETROMINO_TYPE_I,
    J = OBPF_TETROMINO_TYPE_J,
    L = OBPF_TETROMINO_TYPE_L,
    O = OBPF_TETROMINO_TYPE_O,
    S = OBPF_TETROMINO_TYPE_S,
    T = OBPF_TETROMINO_TYPE_T,
    Z = OBPF_TETROMINO_TYPE_Z,
};
