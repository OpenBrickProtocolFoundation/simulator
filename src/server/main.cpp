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
    switch (argc) {
        case 2: {
            auto const lobby_port = parse_integer<std::uint16_t>(argv[1]);
            if (not lobby_port.has_value()) {
                std::cout << std::format("'{}' is not a valid port number\n", argv[1]);
                return EXIT_FAILURE;
            }
            spdlog::info("lobby port = {}", lobby_port.value());
            spdlog::info("starting gameserver");
            auto const server = Server{ lobby_port.value() };
            break;
        }
        case 3: {
            auto const game_server_port = parse_integer<std::uint16_t>(argv[1]);
            if (not game_server_port.has_value()) {
                std::cout << std::format("'{}' is not a valid port number\n", argv[1]);
                return EXIT_FAILURE;
            }
            spdlog::info("game server port = {}", game_server_port.value());
            auto const num_players = parse_integer<std::uint8_t>(argv[2]);
            if (not num_players.has_value() or num_players.value() < 1) {
                std::cout << std::format("'{}' is not a valid number of players\n", argv[2]);
                return EXIT_FAILURE;
            }
            spdlog::info("number of players = {}", num_players.value());
            spdlog::info("starting game server");
            auto const server = Server{ game_server_port.value(), num_players.value() };
            break;
        }
        default:
            std::cout << std::format("Usage: {} [<lobby-port>|<gameserver_port> <num_players>]\n", argv[0]);
            return EXIT_FAILURE;
    }
}
