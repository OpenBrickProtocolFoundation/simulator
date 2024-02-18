#pragma once

#include "message_types.hpp"
#include <cstdint>

struct MessageHeader {
    MessageType type;
    std::uint64_t payload_size;
};
