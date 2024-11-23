#pragma once

#include <chrono>
#include <format>
#include <string>

[[nodiscard]] inline std::string get_current_date_time() {
    auto const now = std::chrono::system_clock::now();
    auto const local_time = std::chrono::zoned_time{ std::chrono::current_zone(), now };
    return std::format("{:%Y-%m-%d_%H%M%S}", local_time);
}
