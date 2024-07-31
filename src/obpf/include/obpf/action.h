#pragma once

#ifdef __cplusplus
extern "C" {
#endif

    typedef enum {
        OBPF_ACTION_ROTATE_CW,
        OBPF_ACTION_ROTATE_CCW,
        OBPF_ACTION_HARD_DROP,
        OBPF_ACTION_TOUCH,
        OBPF_ACTION_CLEAR1,
        OBPF_ACTION_CLEAR2,
        OBPF_ACTION_CLEAR3,
        OBPF_ACTION_CLEAR4,
    } ObpfAction;

#ifdef __cplusplus
}
#endif
