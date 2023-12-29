#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "tetromino.h"
#include "tetromino_type.h"
#include "vec2.h"
#include <stdbool.h>

// Tetrion
struct Tetrion* obpf_create_tetrion(void);
bool obpf_tetrion_try_get_active_tetromino(struct Tetrion const* tetrion, struct ObpfTetromino* out_tetromino);
void obpf_tetrion_simulate_up_until(struct Tetrion* tetrion, uint64_t frame);
void obpf_destroy_tetrion(struct Tetrion const* tetrion);

// Matrix
struct Matrix const* obpf_tetrion_matrix(struct Tetrion const* tetrion);
ObpfTetrominoType obpf_matrix_get(struct Matrix const* matrix, ObpfVec2 position);

#ifdef __cplusplus
}
#endif
