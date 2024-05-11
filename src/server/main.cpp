#include <spdlog/spdlog.h>
#include <concepts>
#include <iostream>
#include <optional>
#include <sockets/sockets.hpp>
#include <string_view>
#include "server.hpp"

template<std::integral Integer>
[[nodiscard]] constexpr std::optional<Integer> parse_integer(std::string_view const chars, int const base = 10) {
    if (chars.empty()) {
        return std::nullopt;
    }
    auto value = Integer{};
    auto const begin = chars.data();
    auto const end = chars.data() + chars.length();
    auto const result = std::from_chars(begin, end, value, base);
    auto const successful = (result.ptr == end and result.ec == std::errc{});
    if (successful) {
        return value;
    }
    return std::nullopt;
}

int main(int const argc, char const* const* const argv) {
    if (argc != 2) {
        std::cout << std::format("Usage: {} <lobby-port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    auto const lobby_port = parse_integer<std::uint16_t>(argv[1]);
    if (not lobby_port.has_value()) {
        std::cout << std::format("'{}' is not a valid port number\n", argv[1]);
        return EXIT_FAILURE;
    }
    spdlog::info("lobby port = {}", lobby_port.value());
    spdlog::info("starting gameserver");
    auto const server = Server{ lobby_port.value() };
}
