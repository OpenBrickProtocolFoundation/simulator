#pragma once

#include <simulator_export.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "input.h"
#include "tetromino.h"
#include "tetromino_type.h"
#include "vec2.h"

    // clang-format off

    // Tetrion
    SIMULATOR_EXPORT struct ObpfTetrion* obpf_create_tetrion(uint64_t seed);
    SIMULATOR_EXPORT void obpf_tetrion_set_lines_cleared_callback(
        struct ObpfTetrion* tetrion,
        void(*callback)(uint8_t count, uint8_t first, uint8_t second, uint8_t third, uint8_t fourth, uint64_t delay)
    );
    SIMULATOR_EXPORT bool obpf_tetrion_try_get_active_tetromino(
        struct ObpfTetrion const* tetrion,
        struct ObpfTetromino* out_tetromino
    );
    SIMULATOR_EXPORT bool obpf_tetrion_try_get_ghost_tetromino(
        struct ObpfTetrion const* tetrion,
        struct ObpfTetromino* out_tetromino
    );
    SIMULATOR_EXPORT void obpf_tetrion_simulate_up_until(struct ObpfTetrion* tetrion, uint64_t frame);
    SIMULATOR_EXPORT void obpf_tetrion_enqueue_event(struct ObpfTetrion* tetrion, ObpfEvent event);
    SIMULATOR_EXPORT void obpf_destroy_tetrion(struct ObpfTetrion const* tetrion);
    SIMULATOR_EXPORT struct ObpfMatrix const* obpf_tetrion_matrix(struct ObpfTetrion const* tetrion);
    SIMULATOR_EXPORT uint8_t obpf_tetrion_width(void);
    SIMULATOR_EXPORT uint8_t obpf_tetrion_height(void);

    // Matrix
    SIMULATOR_EXPORT ObpfTetrominoType obpf_matrix_get(struct ObpfMatrix const* matrix, ObpfVec2 position);

    // clang-format on

#ifdef __cplusplus
}
#endif
