#pragma once

#include <simulator_export.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "input.h"
#include "tetromino.h"
#include "tetromino_type.h"
#include "vec2.h"
#include <stdbool.h>
#include <stdint.h>

// Tetrion
// clang-format off
SIMULATOR_EXPORT struct ObpfTetrion* obpf_create_tetrion(uint64_t seed);
SIMULATOR_EXPORT bool obpf_tetrion_try_get_active_tetromino(struct ObpfTetrion const* tetrion, struct ObpfTetromino* out_tetromino);
SIMULATOR_EXPORT void obpf_tetrion_simulate_up_until(struct ObpfTetrion* tetrion, uint64_t frame);
SIMULATOR_EXPORT void obpf_tetrion_enqueue_event(struct ObpfTetrion* tetrion, ObpfEvent event);
SIMULATOR_EXPORT void obpf_destroy_tetrion(struct ObpfTetrion const* tetrion);
SIMULATOR_EXPORT struct ObpfMatrix const* obpf_tetrion_matrix(struct ObpfTetrion const* tetrion);
SIMULATOR_EXPORT uint8_t obpf_tetrion_width(void);
SIMULATOR_EXPORT uint8_t obpf_tetrion_height(void);
// clang-format on

// Matrix
SIMULATOR_EXPORT ObpfTetrominoType obpf_matrix_get(struct ObpfMatrix const* matrix, ObpfVec2 position);

#ifdef __cplusplus
}
#endif
