#pragma once

enum ObpfKey {
    OBPF_KEY_LEFT,
    OBPF_KEY_RIGHT,
    OBPF_KEY_DROP,
};

enum ObpfEventType {
    OBPF_PRESSED,
    OBPF_RELEASED,
};

struct ObpfEvent {
    ObpfKey key;
    ObpfEventType type;
    uint64_t frame;
};
