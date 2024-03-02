#pragma once

#include <cstdlib>
#include <limits>
#include <cstdint>

inline constexpr auto heartbeat_interval = std::size_t{ 15 };
inline constexpr auto max_payload_size = std::numeric_limits<std::uint16_t>::max();
