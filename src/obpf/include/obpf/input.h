#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

enum ObpfKey {
    OBPF_KEY_LEFT,
    OBPF_KEY_RIGHT,
    OBPF_KEY_DOWN,
    OBPF_KEY_DROP,
    OBPF_KEY_ROTATE_CW,
    OBPF_KEY_ROTATE_CCW,
    OBPF_KEY_HOLD,
};

enum ObpfEventType {
    OBPF_PRESSED,
    OBPF_RELEASED,
};

struct ObpfEvent {
    enum ObpfKey key;
    enum ObpfEventType type;
    uint64_t frame;
};

#ifdef __cplusplus
}
#endif
