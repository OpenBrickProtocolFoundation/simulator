#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t x;
    uint8_t y;
} ObpfVec2;

typedef struct {
    int32_t x;
    int32_t y;
} ObpfVec2i;

#ifdef __cplusplus
}
#endif
