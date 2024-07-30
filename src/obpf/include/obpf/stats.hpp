#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct {
    uint64_t score;
    uint32_t lines_cleared;
    uint32_t level;
} ObpfStats;

#ifdef __cplusplus
}
#endif
