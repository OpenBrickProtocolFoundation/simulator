#pragma once

#include <simulator_export.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "input.h"
#include "rotation.h"
#include "tetromino.h"
#include "tetromino_type.h"
#include "vec2.h"

    typedef struct {
        uint8_t count;
        uint8_t first;
        uint8_t second;
        uint8_t third;
        uint8_t fourth;
        uint64_t countdown;
        uint64_t delay;
    } ObpfLineClearDelayState;

    typedef struct {
        ObpfTetrominoType types[6];
    } ObpfPreviewPieces;

    typedef struct {
        ObpfVec2 positions[4];
    } ObpfMinoPositions;

    // clang-format off

    // Tetrion
    SIMULATOR_EXPORT ObpfMinoPositions obpf_tetromino_get_mino_positions(ObpfTetrominoType type, ObpfRotation rotation);
    SIMULATOR_EXPORT struct ObpfTetrion* obpf_create_tetrion(uint64_t seed);
    SIMULATOR_EXPORT ObpfLineClearDelayState obpf_tetrion_get_line_clear_delay_state(struct ObpfTetrion const* tetrion);
    SIMULATOR_EXPORT bool obpf_tetrion_try_get_active_tetromino(
        struct ObpfTetrion const* tetrion,
        struct ObpfTetromino* out_tetromino
    );
    SIMULATOR_EXPORT bool obpf_tetrion_try_get_ghost_tetromino(
        struct ObpfTetrion const* tetrion,
        struct ObpfTetromino* out_tetromino
    );
    SIMULATOR_EXPORT ObpfPreviewPieces obpf_tetrion_get_preview_pieces(struct ObpfTetrion const* tetrion);
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
