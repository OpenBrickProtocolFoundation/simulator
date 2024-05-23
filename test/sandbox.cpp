#include "utils.hpp"
#include <crapper/crapper.hpp>
#include <crapper/headers.hpp>
#include <network/lobby_server.hpp>
#include <network/message_types.hpp>
#include <network/user.hpp>
#include <sockets/detail/message_buffer.hpp>
#include <sockets/sockets.hpp>

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

int main() {
    static constexpr auto username = "coder2k";
    static constexpr auto password = "secret";

    auto lobby_server = LobbyServer("127.0.0.1", 5000);

    // auto user = lobby_server.register_user("r00tifant", "apple").value();
    auto const user = lobby_server.authenticate(username, password).value();

    // host
    auto lobby = lobby_server.create_lobby(user, LobbySettings{ "coder2k's game", 8 }).value();
    // auto const port = lobby_server.start(user, lobby).value();
    // spdlog::info("gameserver has been started on port {}", std::to_underlying(port));

    // client
    auto const infos = lobby_server.lobbies();
    spdlog::info("Lobbies:\n{}", nlohmann::json(infos).dump(4));
    // auto lobby_info = lobbies.front();

    // auto joined_lobby = lobby_server.join(user, lobby_info);
    // joined_lobby.ready();

    // lobby_server.unregister(user);

    // host
    auto const result = lobby_server.destroy_lobby(user, std::move(lobby));
    assert(result.has_value());
}
