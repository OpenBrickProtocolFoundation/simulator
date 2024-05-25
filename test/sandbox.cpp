#include "utils.hpp"
#include <crapper/crapper.hpp>
#include <crapper/headers.hpp>
#include <network/lobby_server.hpp>
#include <network/message_types.hpp>
#include <network/user.hpp>
#include <sockets/detail/message_buffer.hpp>
#include <sockets/sockets.hpp>
#include <thread>

struct CreateLobbyRequest final {
    std::string name;
    unsigned int size;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CreateLobbyRequest, name, size);

struct CreateLobbyResponse final {
    std::string id;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CreateLobbyResponse, id);

struct StartResponse final {
    std::uint16_t port;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(StartResponse, port);

static void run_second_user() {
    auto lobby_server = LobbyServer("127.0.0.1", 5000);

    // client
    auto const lobby_info = std::invoke([&] {
        while (true) {
            auto const lobby_list = lobby_server.lobbies();
            if (lobby_list.lobbies.empty()) {
                spdlog::info("cannot join lobby yet, since there is no lobby");
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                continue;
            }
            return lobby_list.lobbies.front();
        }
    });

    auto const second_user = lobby_server.authenticate("r00tifant", "apple").value();

    auto const joined_lobby = lobby_server.join(second_user, lobby_info);

    auto const response = lobby_server.set_ready(second_user, joined_lobby.value());
    auto const gameserver_port = response.value();

    spdlog::info("the client should now connect to the server on port: {}", std::to_underlying(gameserver_port));
}

int main() {
    auto const client_thread = std::jthread{ run_second_user };

    static constexpr auto username = "coder2k";
    static constexpr auto password = "secret";

    auto lobby_server = LobbyServer("127.0.0.1", 5000);

    // auto user = lobby_server.register_user("r00tifant", "apple").value();
    auto const user = lobby_server.authenticate(username, password).value();

    // host
    auto lobby = lobby_server.create_lobby(user, LobbySettings{ "coder2k's game", 8 }).value();

    std::this_thread::sleep_for(std::chrono::seconds(2));

    auto const port = std::invoke([&] {
        while (true) {
            auto const response = lobby_server.start(user, lobby);
            if (response.has_value()) {
                return response.value();
            }
            if (response.error() == GameStartError::NotAllPlayersReady) {
                spdlog::info("cannot start game yet since not all players are ready");
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                continue;
            }
            throw std::runtime_error{ "failed to start game" };
        }
    });
    spdlog::info("gameserver has been started on port {}", std::to_underlying(port));

    // lobby_server.unregister(user);

    std::this_thread::sleep_for(std::chrono::seconds(2));

    // host
    auto const result = lobby_server.destroy_lobby(user, std::move(lobby));
    assert(result.has_value());
}
