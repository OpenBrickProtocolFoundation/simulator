#pragma once

#include <lib2k/types.hpp>
#include <tl/optional.hpp>
#include <vector>

struct GarbageSendEvent {
    u64 frame;
    u8 num_lines;

    explicit constexpr GarbageSendEvent(u64 const frame, u8 const num_lines)
        : frame{ frame }, num_lines{ num_lines } {}
};

struct ObpfTetrion;

[[nodiscard]] tl::optional<ObpfTetrion&> determine_garbage_target(
    std::vector<ObpfTetrion*> const& tetrions,
    u8 sender_tetrion_id,
    u64 frame
);
