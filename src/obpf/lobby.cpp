#include <obpf/lobby.h>
#include <iostream>
#include <network/lobby_server.hpp>

ObpfLobbyServerConnection* obpf_create_lobby_server_connection(
    char const* const host,
    uint16_t const port
) {
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
) {  // clang-format on
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
) {  // clang-format on
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

ObpfLobbyList const* obpf_get_lobby_list(ObpfLobbyServerConnection* lobby_server_connection) {
    auto& connection = *reinterpret_cast<LobbyServerConnection*>(lobby_server_connection);
    auto const lobbies = new LobbyList{ connection.lobbies() };
    return reinterpret_cast<ObpfLobbyList const*>(lobbies);
}

void obpf_free_lobby_list(ObpfLobbyList const* lobby_list) {
    delete reinterpret_cast<LobbyList const*>(lobby_list);
}

size_t obpf_lobby_list_length(ObpfLobbyList const* lobby_list) {
    return reinterpret_cast<LobbyList const*>(lobby_list)->lobbies.size();
}

ObpfLobbyInfo const* obpf_lobby_list_at(ObpfLobbyList const* lobby_list, size_t index) {
    if (index >= obpf_lobby_list_length(lobby_list)) {
        return nullptr;
    }
    return reinterpret_cast<ObpfLobbyInfo const*>(
        &reinterpret_cast<LobbyList const*>(lobby_list)->lobbies.at(index)
    );
}

char const* obpf_lobby_info_id(ObpfLobbyInfo const* lobby_info) {
    return reinterpret_cast<LobbyInfo const*>(lobby_info)->id.c_str();
}

char const* obpf_lobby_info_name(ObpfLobbyInfo const* lobby_info) {
    return reinterpret_cast<LobbyInfo const*>(lobby_info)->name.c_str();
}

uint16_t obpf_lobby_info_size(ObpfLobbyInfo const* lobby_info) {
    return reinterpret_cast<LobbyInfo const*>(lobby_info)->size;
}

uint16_t obpf_lobby_info_num_players_in_lobby(ObpfLobbyInfo const* lobby_info) {
    return reinterpret_cast<LobbyInfo const*>(lobby_info)->num_players_in_lobby;
}

char const* obpf_lobby_info_host_id(ObpfLobbyInfo const* lobby_info) {
    return reinterpret_cast<LobbyInfo const*>(lobby_info)->host_info.id.c_str();
}

char const* obpf_lobby_info_host_name(ObpfLobbyInfo const* lobby_info) {
    return reinterpret_cast<LobbyInfo const*>(lobby_info)->host_info.name.c_str();
}

// clang-format off
ObpfLobby* obpf_lobby_connection_join(
    ObpfLobbyServerConnection* lobby_server_connection,
    ObpfLobbyInfo const* lobby_info,
    ObpfLobbyUser const* lobby_user
) {  // clang-format on
    auto& connection = *reinterpret_cast<LobbyServerConnection*>(lobby_server_connection);
    auto const& user = *reinterpret_cast<User const*>(lobby_user);
    auto const& info = *reinterpret_cast<LobbyInfo const*>(lobby_info);
    std::cerr << "before calling connection.join\n";
    auto result = connection.join(user, info);
    std::cerr << std::boolalpha << "result has value? " << result.has_value() << '\n';
    if (not result.has_value()) {
        return nullptr;
    }
    return reinterpret_cast<ObpfLobby*>(new Lobby{ std::move(result).value() });
}

// clang-format off
uint16_t obpf_lobby_set_ready(
    ObpfLobbyServerConnection* lobby_server_connection,
    ObpfLobbyUser const* lobby_user,
    ObpfLobby* obpf_lobby
) {  // clang-format on
    auto& connection = *reinterpret_cast<LobbyServerConnection*>(lobby_server_connection);
    auto const& user = *reinterpret_cast<User const*>(lobby_user);
    auto const& lobby = *reinterpret_cast<Lobby const*>(obpf_lobby);
    auto const result = connection.set_ready(user, lobby);
    assert(result.has_value());
    return std::to_underlying(result.value());
}

ObpfLobbyDetails const* obpf_lobby_connection_get_lobby_details(
    ObpfLobbyServerConnection* lobby_server_connection,
    ObpfLobbyInfo const* lobby_info,
    ObpfLobbyUser const* lobby_user
) {
    std::cerr << "function called\n";
    auto& connection = *reinterpret_cast<LobbyServerConnection*>(lobby_server_connection);
    std::cerr << "connection\n";
    auto const& info = *reinterpret_cast<LobbyInfo const*>(lobby_info);
    std::cerr << "info\n";
    auto const& user = *reinterpret_cast<User const*>(lobby_user);
    std::cerr << "user\n";
    auto result = connection.lobby_details(user, info);
    std::cerr << "result received\n";
    if (not result.has_value()) {
        std::cerr << "no value\n";
        return nullptr;
    }
    return reinterpret_cast<ObpfLobbyDetails const*>(new LobbyDetails{ std::move(result).value() });
}

void obpf_free_lobby_details(ObpfLobbyDetails const* lobby_details) {
    delete reinterpret_cast<LobbyDetails const*>(lobby_details);
}

char const* obpf_lobby_details_id(ObpfLobbyDetails const* lobby_details) {
    return reinterpret_cast<LobbyDetails const*>(lobby_details)->id.c_str();
}

char const* obpf_lobby_details_name(ObpfLobbyDetails const* lobby_details) {
    return reinterpret_cast<LobbyDetails const*>(lobby_details)->name.c_str();
}

uint16_t obpf_lobby_details_size(ObpfLobbyDetails const* lobby_details) {
    return reinterpret_cast<LobbyDetails const*>(lobby_details)->size;
}

size_t obpf_lobby_details_num_clients(ObpfLobbyDetails const* lobby_details) {
    return reinterpret_cast<LobbyDetails const*>(lobby_details)->client_infos.size();
}

char const* obpf_lobby_details_client_id(ObpfLobbyDetails const* lobby_details, size_t index) {
    if (index >= obpf_lobby_details_num_clients(lobby_details)) {
        return nullptr;
    }
    return reinterpret_cast<LobbyDetails const*>(lobby_details)->client_infos.at(index).id.c_str();
}

char const* obpf_lobby_details_client_name(ObpfLobbyDetails const* lobby_details, size_t index) {
    if (index >= obpf_lobby_details_num_clients(lobby_details)) {
        return nullptr;
    }
    return reinterpret_cast<LobbyDetails const*>(lobby_details)->client_infos.at(index).name.c_str();
}

bool obpf_lobby_details_client_is_ready(ObpfLobbyDetails const* lobby_details, size_t index) {
    if (index >= obpf_lobby_details_num_clients(lobby_details)) {
        return false;
    }
    return reinterpret_cast<LobbyDetails const*>(lobby_details)->client_infos.at(index).is_ready;
}

char const* obpf_lobby_details_host_id(ObpfLobbyDetails const* lobby_details) {
    return reinterpret_cast<LobbyDetails const*>(lobby_details)->host_info.id.c_str();
}

char const* obpf_lobby_details_host_name(ObpfLobbyDetails const* lobby_details) {
    return reinterpret_cast<LobbyDetails const*>(lobby_details)->host_info.name.c_str();
}
