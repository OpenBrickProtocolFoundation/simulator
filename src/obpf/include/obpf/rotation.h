#pragma once

#ifdef __cplusplus
extern "C" {
#endif

    typedef enum {
        OBPF_ROTATION_NORTH = 0,
        OBPF_ROTATION_EAST,
        OBPF_ROTATION_SOUTH,
        OBPF_ROTATION_WEST,
        OBPF_ROTATION_LAST_ROTATION = OBPF_ROTATION_WEST,
    } ObpfRotation;

#ifdef __cplusplus
}
#endif
