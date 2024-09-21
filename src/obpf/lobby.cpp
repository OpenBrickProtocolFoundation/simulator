#include <obpf/lobby.h>
#include <spdlog/spdlog.h>
#include <iostream>
#include <network/lobby_server.hpp>

ObpfLobbyServerConnection* obpf_create_lobby_server_connection(char const* const host, uint16_t const port) try {
    return reinterpret_cast<ObpfLobbyServerConnection*>(new LobbyServerConnection{ host, port });
} catch (std::exception const& e) {

    spdlog::error("Failed to create lobby server connection: {}", e.what());
    return nullptr;
} catch (...) {
    spdlog::error("Failed to create lobby server connection: unknown error");
    return nullptr;
}

void obpf_destroy_lobby_server_connection(ObpfLobbyServerConnection const* lobby_server_connection) try {
    delete reinterpret_cast<LobbyServerConnection const*>(lobby_server_connection);
} catch (std::exception const& e) {

    spdlog::error("Failed to destroy lobby server connection: {}", e.what());
} catch (...) {
    spdlog::error("Failed to destroy lobby server connection: unknown error");
}

// clang-format off
ObpfLobbyUser* obpf_lobby_connection_register_user(
    ObpfLobbyServerConnection* const lobby_server_connection,
    char const* const username,
    char const* const password
) try {  // clang-format on
    auto& connection = *reinterpret_cast<LobbyServerConnection*>(lobby_server_connection);
    if (auto const user = connection.register_user(username, password)) {
        return reinterpret_cast<ObpfLobbyUser*>(new User{ user.value() });
    }
    return nullptr;
} catch (std::exception const& e) {

    spdlog::error("Failed to register user: {}", e.what());
    return nullptr;
} catch (...) {
    spdlog::error("Failed to register user: unknown error");
    return nullptr;
}

ObpfLobbyUser* obpf_lobby_connection_authenticate_user(
    ObpfLobbyServerConnection* const lobby_server_connection,
    char const* const username,
    char const* const password
) try {
    auto& connection = *reinterpret_cast<LobbyServerConnection*>(lobby_server_connection);
    if (auto const user = connection.authenticate(username, password)) {
        return reinterpret_cast<ObpfLobbyUser*>(new User{ user.value() });
    }
    return nullptr;
} catch (std::exception const& e) {

    spdlog::error("Failed to authenticate user: {}", e.what());
    return nullptr;
} catch (...) {
    spdlog::error("Failed to authenticate user: unknown error");
    return nullptr;
}

void obpf_user_destroy(ObpfLobbyUser const* const user) try {
    delete reinterpret_cast<User const*>(user);
} catch (std::exception const& e) {

    spdlog::error("Failed to destroy user: {}", e.what());
} catch (...) {
    spdlog::error("Failed to destroy user: unknown error");
}

// clang-format off
void obpf_lobby_unregister_user(
    ObpfLobbyServerConnection* const lobby_server_connection,
    ObpfLobbyUser** const user_pointer
) try {  // clang-format on
    auto& connection = *reinterpret_cast<LobbyServerConnection*>(lobby_server_connection);
    connection.unregister(*reinterpret_cast<User*>(*user_pointer));
    obpf_user_destroy(*user_pointer);
    *user_pointer = nullptr;
} catch (std::exception const& e) {

    spdlog::error("Failed to unregister user: {}", e.what());
} catch (...) {
    spdlog::error("Failed to unregister user: unknown error");
}

ObpfLobby* obpf_lobby_connection_create_lobby(
    ObpfLobbyServerConnection* const lobby_server_connection,
    ObpfLobbyUser const* const user,
    char const* const lobby_name,
    uint16_t const lobby_size
) try {
    auto& connection = *reinterpret_cast<LobbyServerConnection*>(lobby_server_connection);
    auto const settings = LobbySettings{ lobby_name, lobby_size };
    if (auto const lobby = connection.create_lobby(*reinterpret_cast<User const*>(user), settings)) {
        return reinterpret_cast<ObpfLobby*>(new Lobby{ lobby.value() });
    }
    return nullptr;
} catch (std::exception const& e) {

    spdlog::error("Failed to create lobby: {}", e.what());
    return nullptr;
} catch (...) {
    spdlog::error("Failed to create lobby: unknown error");
    return nullptr;
}

bool obpf_lobby_connection_destroy_lobby(
    ObpfLobbyServerConnection* const lobby_server_connection,
    ObpfLobbyUser const* const lobby_user,
    ObpfLobby** const obpf_lobby
) try {
    auto& connection = *reinterpret_cast<LobbyServerConnection*>(lobby_server_connection);
    auto const& user = *reinterpret_cast<User const*>(lobby_user);
    auto lobby = *reinterpret_cast<Lobby*>(*obpf_lobby);
    auto const result = static_cast<bool>(connection.destroy_lobby(user, std::move(lobby)));
    delete reinterpret_cast<Lobby*>(*obpf_lobby);
    *obpf_lobby = nullptr;
    return result;
} catch (std::exception const& e) {

    spdlog::error("Failed to destroy lobby: {}", e.what());
    return false;
} catch (...) {
    spdlog::error("Failed to destroy lobby: unknown error");
    return false;
}

bool obpf_lobby_connection_start_lobby(
    ObpfLobbyServerConnection* const lobby_server_connection,
    ObpfLobbyUser const* const lobby_user,
    ObpfLobby const* const obpf_lobby,
    uint16_t* const out_server_port
) try {
    auto& connection = *reinterpret_cast<LobbyServerConnection*>(lobby_server_connection);
    auto const& user = *reinterpret_cast<User const*>(lobby_user);
    auto const& lobby = *reinterpret_cast<Lobby const*>(obpf_lobby);
    auto const result = connection.start(user, lobby);
    if (result.has_value()) {
        *out_server_port = std::to_underlying(result.value());
        return true;
    }
    return false;
} catch (std::exception const& e) {

    spdlog::error("Failed to start lobby: {}", e.what());
    return false;
} catch (...) {
    spdlog::error("Failed to start lobby: unknown error");
    return false;
}

ObpfLobbyList const* obpf_get_lobby_list(ObpfLobbyServerConnection* lobby_server_connection) try {
    auto& connection = *reinterpret_cast<LobbyServerConnection*>(lobby_server_connection);
    auto const lobbies = new LobbyList{ connection.lobbies() };
    return reinterpret_cast<ObpfLobbyList const*>(lobbies);
} catch (std::exception const& e) {

    spdlog::error("Failed to get lobby list: {}", e.what());
    return nullptr;
} catch (...) {
    spdlog::error("Failed to get lobby list: unknown error");
    return nullptr;
}

void obpf_free_lobby_list(ObpfLobbyList const* lobby_list) try {
    delete reinterpret_cast<LobbyList const*>(lobby_list);
} catch (std::exception const& e) {

    spdlog::error("Failed to free lobby list: {}", e.what());
} catch (...) {
    spdlog::error("Failed to free lobby list: unknown error");
}

size_t obpf_lobby_list_length(ObpfLobbyList const* lobby_list) try {
    return reinterpret_cast<LobbyList const*>(lobby_list)->lobbies.size();
} catch (std::exception const& e) {

    spdlog::error("Failed to get lobby list length: {}", e.what());
    return 0;
} catch (...) {
    spdlog::error("Failed to get lobby list length: unknown error");
    return 0;
}

ObpfLobbyInfo const* obpf_lobby_list_at(ObpfLobbyList const* lobby_list, size_t index) try {
    if (index >= obpf_lobby_list_length(lobby_list)) {
        return nullptr;
    }
    return reinterpret_cast<ObpfLobbyInfo const*>(&reinterpret_cast<LobbyList const*>(lobby_list)->lobbies.at(index));
} catch (std::exception const& e) {

    spdlog::error("Failed to get lobby info at index: {}", e.what());
    return nullptr;
} catch (...) {
    spdlog::error("Failed to get lobby info at index: unknown error");
    return nullptr;
}

char const* obpf_lobby_info_id(ObpfLobbyInfo const* lobby_info) try {
    return reinterpret_cast<LobbyInfo const*>(lobby_info)->id.c_str();
} catch (std::exception const& e) {

    spdlog::error("Failed to get lobby info id: {}", e.what());
    return nullptr;
} catch (...) {
    spdlog::error("Failed to get lobby info id: unknown error");
    return nullptr;
}

char const* obpf_lobby_info_name(ObpfLobbyInfo const* lobby_info) try {
    return reinterpret_cast<LobbyInfo const*>(lobby_info)->name.c_str();
} catch (std::exception const& e) {

    spdlog::error("Failed to get lobby info name: {}", e.what());
    return nullptr;
} catch (...) {
    spdlog::error("Failed to get lobby info name: unknown error");
    return nullptr;
}

uint16_t obpf_lobby_info_size(ObpfLobbyInfo const* lobby_info) try {
    return reinterpret_cast<LobbyInfo const*>(lobby_info)->size;
} catch (std::exception const& e) {

    spdlog::error("Failed to get lobby info size: {}", e.what());
    return 0;
} catch (...) {
    spdlog::error("Failed to get lobby info size: unknown error");
    return 0;
}

uint16_t obpf_lobby_info_num_players_in_lobby(ObpfLobbyInfo const* lobby_info) try {
    return reinterpret_cast<LobbyInfo const*>(lobby_info)->num_players_in_lobby;
} catch (std::exception const& e) {

    spdlog::error("Failed to get lobby info num players in lobby: {}", e.what());
    return 0;
} catch (...) {
    spdlog::error("Failed to get lobby info num players in lobby: unknown error");
    return 0;
}

char const* obpf_lobby_info_host_id(ObpfLobbyInfo const* lobby_info) try {
    return reinterpret_cast<LobbyInfo const*>(lobby_info)->host_info.id.c_str();
} catch (std::exception const& e) {

    spdlog::error("Failed to get lobby info host id: {}", e.what());
    return nullptr;
} catch (...) {
    spdlog::error("Failed to get lobby info host id: unknown error");
    return nullptr;
}

char const* obpf_lobby_info_host_name(ObpfLobbyInfo const* lobby_info) try {
    return reinterpret_cast<LobbyInfo const*>(lobby_info)->host_info.name.c_str();
} catch (std::exception const& e) {

    spdlog::error("Failed to get lobby info host name: {}", e.what());
    return nullptr;
} catch (...) {
    spdlog::error("Failed to get lobby info host name: unknown error");
    return nullptr;
}

// clang-format off
ObpfLobby* obpf_lobby_connection_join(
    ObpfLobbyServerConnection* lobby_server_connection,
    ObpfLobbyInfo const* lobby_info,
    ObpfLobbyUser const* lobby_user
) try {  // clang-format on
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
} catch (std::exception const& e) {

    spdlog::error("Failed to join lobby: {}", e.what());
    return nullptr;
} catch (...) {
    spdlog::error("Failed to join lobby: unknown error");
    return nullptr;
}

// clang-format off
uint16_t obpf_lobby_set_ready(
    ObpfLobbyServerConnection* lobby_server_connection,
    ObpfLobbyUser const* lobby_user,
    ObpfLobby* obpf_lobby
) try {  // clang-format on
    auto& connection = *reinterpret_cast<LobbyServerConnection*>(lobby_server_connection);
    auto const& user = *reinterpret_cast<User const*>(lobby_user);
    auto const& lobby = *reinterpret_cast<Lobby const*>(obpf_lobby);
    auto const result = connection.set_ready(user, lobby);
    assert(result.has_value());
    return std::to_underlying(result.value());
} catch (std::exception const& e) {

    spdlog::error("Failed to set ready: {}", e.what());
    return 0;
} catch (...) {
    spdlog::error("Failed to set ready: unknown error");
    return 0;
}

ObpfLobbyDetails const* obpf_lobby_connection_get_lobby_details(
    ObpfLobbyServerConnection* lobby_server_connection,
    ObpfLobbyInfo const* lobby_info,
    ObpfLobbyUser const* lobby_user
) try {
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
} catch (std::exception const& e) {

    spdlog::error("Failed to get lobby details: {}", e.what());
    return nullptr;
} catch (...) {
    spdlog::error("Failed to get lobby details: unknown error");
    return nullptr;
}

void obpf_free_lobby_details(ObpfLobbyDetails const* lobby_details) try {
    delete reinterpret_cast<LobbyDetails const*>(lobby_details);
} catch (std::exception const& e) {

    spdlog::error("Failed to free lobby details: {}", e.what());
} catch (...) {
    spdlog::error("Failed to free lobby details: unknown error");
}

char const* obpf_lobby_details_id(ObpfLobbyDetails const* lobby_details) try {
    return reinterpret_cast<LobbyDetails const*>(lobby_details)->id.c_str();
} catch (std::exception const& e) {

    spdlog::error("Failed to get lobby details id: {}", e.what());
    return nullptr;
} catch (...) {
    spdlog::error("Failed to get lobby details id: unknown error");
    return nullptr;
}

char const* obpf_lobby_details_name(ObpfLobbyDetails const* lobby_details) try {
    return reinterpret_cast<LobbyDetails const*>(lobby_details)->name.c_str();
} catch (std::exception const& e) {

    spdlog::error("Failed to get lobby details name: {}", e.what());
    return nullptr;
} catch (...) {
    spdlog::error("Failed to get lobby details name: unknown error");
    return nullptr;
}

uint16_t obpf_lobby_details_size(ObpfLobbyDetails const* lobby_details) try {
    return reinterpret_cast<LobbyDetails const*>(lobby_details)->size;
} catch (std::exception const& e) {

    spdlog::error("Failed to get lobby details size: {}", e.what());
    return 0;
} catch (...) {
    spdlog::error("Failed to get lobby details size: unknown error");
    return 0;
}

size_t obpf_lobby_details_num_clients(ObpfLobbyDetails const* lobby_details) try {
    return reinterpret_cast<LobbyDetails const*>(lobby_details)->client_infos.size();
} catch (std::exception const& e) {

    spdlog::error("Failed to get lobby details num clients: {}", e.what());
    return 0;
} catch (...) {
    spdlog::error("Failed to get lobby details num clients: unknown error");
    return 0;
}

char const* obpf_lobby_details_client_id(ObpfLobbyDetails const* lobby_details, size_t index) try {
    if (index >= obpf_lobby_details_num_clients(lobby_details)) {
        return nullptr;
    }
    return reinterpret_cast<LobbyDetails const*>(lobby_details)->client_infos.at(index).id.c_str();
} catch (std::exception const& e) {

    spdlog::error("Failed to get lobby details client id: {}", e.what());
    return nullptr;
} catch (...) {
    spdlog::error("Failed to get lobby details client id: unknown error");
    return nullptr;
}

char const* obpf_lobby_details_client_name(ObpfLobbyDetails const* lobby_details, size_t index) try {
    if (index >= obpf_lobby_details_num_clients(lobby_details)) {
        return nullptr;
    }
    return reinterpret_cast<LobbyDetails const*>(lobby_details)->client_infos.at(index).name.c_str();
} catch (std::exception const& e) {

    spdlog::error("Failed to get lobby details client name: {}", e.what());
    return nullptr;
} catch (...) {
    spdlog::error("Failed to get lobby details client name: unknown error");
    return nullptr;
}

bool obpf_lobby_details_client_is_ready(ObpfLobbyDetails const* lobby_details, size_t index) try {
    if (index >= obpf_lobby_details_num_clients(lobby_details)) {
        return false;
    }
    return reinterpret_cast<LobbyDetails const*>(lobby_details)->client_infos.at(index).is_ready;
} catch (std::exception const& e) {

    spdlog::error("Failed to get lobby details client is ready: {}", e.what());
    return false;
} catch (...) {
    spdlog::error("Failed to get lobby details client is ready: unknown error");
    return false;
}

char const* obpf_lobby_details_host_id(ObpfLobbyDetails const* lobby_details) try {
    return reinterpret_cast<LobbyDetails const*>(lobby_details)->host_info.id.c_str();
} catch (std::exception const& e) {

    spdlog::error("Failed to get lobby details host id: {}", e.what());
    return nullptr;
} catch (...) {
    spdlog::error("Failed to get lobby details host id: unknown error");
    return nullptr;
}

char const* obpf_lobby_details_host_name(ObpfLobbyDetails const* lobby_details) try {
    return reinterpret_cast<LobbyDetails const*>(lobby_details)->host_info.name.c_str();
} catch (std::exception const& e) {

    spdlog::error("Failed to get lobby details host name: {}", e.what());
    return nullptr;
} catch (...) {
    spdlog::error("Failed to get lobby details host name: unknown error");
    return nullptr;
}
