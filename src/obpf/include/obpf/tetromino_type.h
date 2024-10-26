#pragma once

#ifdef __cplusplus
extern "C" {
#endif

    typedef enum {
        OBPF_TETROMINO_TYPE_EMPTY = 0,
        OBPF_TETROMINO_TYPE_I,
        OBPF_TETROMINO_TYPE_J,
        OBPF_TETROMINO_TYPE_L,
        OBPF_TETROMINO_TYPE_O,
        OBPF_TETROMINO_TYPE_S,
        OBPF_TETROMINO_TYPE_T,
        OBPF_TETROMINO_TYPE_Z,
        OBPF_TETROMINO_TYPE_GARBAGE,
        OBPF_TETROMINO_TYPE_LAST = OBPF_TETROMINO_TYPE_GARBAGE,
    } ObpfTetrominoType;

#ifdef __cplusplus
}
#endif
