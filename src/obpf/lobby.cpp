#include <network/lobby_server.hpp>
#include <obpf/lobby.h>

ObpfLobbyServerConnection* obpf_create_lobby_server_connection(char const* const host, uint16_t const port) {
    return reinterpret_cast<ObpfLobbyServerConnection*>(new LobbyServerConnection{ host, port });
}

void obpf_destroy_lobby_server_connection(ObpfLobbyServerConnection const* lobby_server_connection) {
    delete reinterpret_cast<LobbyServerConnection const*>(lobby_server_connection);
}

// clang-format off
ObpfLobbyUser* obpf_lobby_connection_register_user(
    ObpfLobbyServerConnection* const lobby_server_connection,
    char const* const username,
    char const* const password
) { // clang-format on
    auto& connection = *reinterpret_cast<LobbyServerConnection*>(lobby_server_connection);
    if (auto const user = connection.register_user(username, password)) {
        return reinterpret_cast<ObpfLobbyUser*>(new User{ user.value() });
    }
    return nullptr;
}

ObpfLobbyUser* obpf_lobby_connection_authenticate_user(
        ObpfLobbyServerConnection* const lobby_server_connection,
        char const* const username,
        char const* const password
) {
    auto& connection = *reinterpret_cast<LobbyServerConnection*>(lobby_server_connection);
    if (auto const user = connection.authenticate(username, password)) {
        return reinterpret_cast<ObpfLobbyUser*>(new User{ user.value() });
    }
    return nullptr;
}

void obpf_user_destroy(ObpfLobbyUser const* const user) {
    delete reinterpret_cast<User const*>(user);
}

// clang-format off
void obpf_lobby_unregister_user(
    ObpfLobbyServerConnection* const lobby_server_connection,
    ObpfLobbyUser** const user_pointer
) { // clang-format on
    auto& connection = *reinterpret_cast<LobbyServerConnection*>(lobby_server_connection);
    connection.unregister(*reinterpret_cast<User*>(*user_pointer));
    obpf_user_destroy(*user_pointer);
    *user_pointer = nullptr;
}

ObpfLobby* obpf_lobby_connection_create_lobby(
        ObpfLobbyServerConnection* const lobby_server_connection,
        ObpfLobbyUser const* const user,
        char const* const lobby_name,
        uint16_t const lobby_size
) {
    auto& connection = *reinterpret_cast<LobbyServerConnection*>(lobby_server_connection);
    auto const settings = LobbySettings{ lobby_name, lobby_size };
    if (auto const lobby = connection.create_lobby(*reinterpret_cast<User const*>(user), settings)) {
        return reinterpret_cast<ObpfLobby*>(new Lobby{ lobby.value() });
    }
    return nullptr;
}

bool obpf_lobby_connection_destroy_lobby(
        ObpfLobbyServerConnection* const lobby_server_connection,
        ObpfLobbyUser const* const lobby_user,
        ObpfLobby** const obpf_lobby
) {
    auto& connection = *reinterpret_cast<LobbyServerConnection*>(lobby_server_connection);
    auto const& user = *reinterpret_cast<User const*>(lobby_user);
    auto lobby = *reinterpret_cast<Lobby*>(*obpf_lobby);
    auto const result = static_cast<bool>(connection.destroy_lobby(user, std::move(lobby)));
    delete reinterpret_cast<Lobby*>(*obpf_lobby);
    *obpf_lobby = nullptr;
    return result;
}

bool obpf_lobby_connection_start_lobby(
        ObpfLobbyServerConnection* const lobby_server_connection,
        ObpfLobbyUser const* const lobby_user,
        ObpfLobby const* const obpf_lobby,
        uint16_t* const out_server_port
) {
    auto& connection = *reinterpret_cast<LobbyServerConnection*>(lobby_server_connection);
    auto const& user = *reinterpret_cast<User const*>(lobby_user);
    auto const& lobby = *reinterpret_cast<Lobby const*>(obpf_lobby);
    auto const result = connection.start(user, lobby);
    if (result.has_value()) {
        *out_server_port = std::to_underlying(result.value());
        return true;
    }
    return false;
}
