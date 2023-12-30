#pragma once

#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "tetromino.h"
#include "tetromino_type.h"
#include "vec2.h"
#include "input.h"
#include <stdbool.h>

// Tetrion
EXPORT struct Tetrion* obpf_create_tetrion(void);
EXPORT bool obpf_tetrion_try_get_active_tetromino(struct Tetrion const* tetrion, struct ObpfTetromino* out_tetromino);
EXPORT void obpf_tetrion_simulate_up_until(struct Tetrion* tetrion, uint64_t frame);
EXPORT void obpf_tetrion_enqueue_event(struct Tetrion* tetrion, ObpfEvent event);
EXPORT void obpf_destroy_tetrion(struct Tetrion const* tetrion);
EXPORT struct Matrix const* obpf_tetrion_matrix(struct Tetrion const* tetrion);
EXPORT uint8_t obpf_tetrion_width(void);
EXPORT uint8_t obpf_tetrion_height(void);

// Matrix
EXPORT ObpfTetrominoType obpf_matrix_get(struct Matrix const* matrix, ObpfVec2 position);

#ifdef __cplusplus
}
#endif
