#pragma once

#include <obpf_export.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <common/common.h>
#include <stdbool.h>
#include <stdint.h>
#include "input.h"
#include "rotation.h"
#include "stats.h"
#include "tetromino.h"
#include "tetromino_type.h"
#include "vec2.h"

    typedef struct {
        uint8_t count;
        uint8_t lines[4];
        uint64_t countdown;
        uint64_t delay;
    } ObpfLineClearDelayState;

    typedef struct {
        ObpfTetrominoType types[6];
    } ObpfPreviewPieces;

    typedef struct {
        ObpfVec2 positions[4];
    } ObpfMinoPositions;

    typedef struct {
        uint8_t bitmask;
    } ObpfKeyState;

    // clang-format off

    // Tetrion
    OBPF_EXPORT ObpfMinoPositions obpf_tetromino_get_mino_positions(ObpfTetrominoType type, ObpfRotation rotation);
    OBPF_EXPORT ObpfKeyState obpf_key_state_create(
        bool left,
        bool right,
        bool down,
        bool drop,
        bool rotate_clockwise,
        bool rotate_counter_clockwise,
        bool hold
    );

    struct ObpfObserverList {
        size_t num_observers;
        struct ObpfTetrion** observers;
    };

    OBPF_EXPORT struct ObpfTetrion* obpf_create_tetrion(uint64_t seed);
    OBPF_EXPORT struct ObpfTetrion* obpf_create_multiplayer_tetrion(const char* host, uint16_t port);
    OBPF_EXPORT struct ObpfObserverList obpf_tetrion_get_observers(struct ObpfTetrion const* tetrion);
    OBPF_EXPORT void obpf_destroy_observers(struct ObpfObserverList observers);
    OBPF_EXPORT struct ObpfTetrion* obpf_clone_tetrion(struct ObpfTetrion const* tetrion);
    OBPF_EXPORT void obpf_tetrion_set_action_handler(
        struct ObpfTetrion* tetrion,
        ObpfActionHandler handler,
        void* user_data
    );
    OBPF_EXPORT ObpfStats obpf_tetrion_get_stats(struct ObpfTetrion const* tetrion);
    OBPF_EXPORT bool obpf_tetrion_is_game_over(struct ObpfTetrion const* tetrion);
    OBPF_EXPORT ObpfLineClearDelayState obpf_tetrion_get_line_clear_delay_state(struct ObpfTetrion const* tetrion);
    OBPF_EXPORT bool obpf_tetrion_try_get_active_tetromino(
        struct ObpfTetrion const* tetrion,
        struct ObpfTetromino* out_tetromino
    );
    OBPF_EXPORT bool obpf_tetrion_try_get_active_tetromino_transform(
        struct ObpfTetrion const* tetrion,
        ObpfTetrominoType* out_type,
        ObpfRotation* out_rotation,
        ObpfVec2i* out_position
    );
    OBPF_EXPORT bool obpf_tetrion_try_get_ghost_tetromino(
        struct ObpfTetrion const* tetrion,
        struct ObpfTetromino* out_tetromino
    );
    OBPF_EXPORT ObpfPreviewPieces obpf_tetrion_get_preview_pieces(struct ObpfTetrion const* tetrion);
    OBPF_EXPORT ObpfTetrominoType obpf_tetrion_get_hold_piece(struct ObpfTetrion const* tetrion);
    OBPF_EXPORT uint64_t obpf_tetrion_get_next_frame(struct ObpfTetrion const* tetrion);
    OBPF_EXPORT void obpf_tetrion_simulate_next_frame(struct ObpfTetrion* tetrion, ObpfKeyState key_state);
    OBPF_EXPORT void obpf_destroy_tetrion(struct ObpfTetrion const* tetrion);
    OBPF_EXPORT uint8_t obpf_tetrion_width(void);
    OBPF_EXPORT uint8_t obpf_tetrion_height(void);
    OBPF_EXPORT uint8_t obpf_tetrion_num_invisible_lines(void);

    // Matrix
    OBPF_EXPORT ObpfTetrominoType obpf_tetrion_matrix_get(const struct ObpfTetrion* tetrion, ObpfVec2 position);

    // clang-format on

#ifdef __cplusplus
}
#endif
