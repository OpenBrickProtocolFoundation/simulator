#pragma once

#include <simulator_export.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// clang-format off

// lobby-server-related functions
SIMULATOR_EXPORT struct ObpfLobbyServerConnection* obpf_create_lobby_server_connection(char const* host, uint16_t port);
SIMULATOR_EXPORT void obpf_destroy_lobby_server_connection(struct ObpfLobbyServerConnection const* lobby_server_connection);

// user-related functions
SIMULATOR_EXPORT struct ObpfLobbyUser* obpf_lobby_connection_register_user(
    struct ObpfLobbyServerConnection* lobby_server_connection,
    const char* username,
    const char* password
);
SIMULATOR_EXPORT struct ObpfLobbyUser* obpf_lobby_connection_authenticate_user(
    struct ObpfLobbyServerConnection* lobby_server_connection,
    const char* username,
    const char* password
);
SIMULATOR_EXPORT void obpf_lobby_unregister_user(
    struct ObpfLobbyServerConnection* lobby_server_connection,
    struct ObpfLobbyUser** user_pointer
);
SIMULATOR_EXPORT void obpf_user_destroy(struct ObpfLobbyUser const* user);

// lobby-related functions
SIMULATOR_EXPORT struct ObpfLobby* obpf_lobby_connection_create_lobby(
    struct ObpfLobbyServerConnection* lobby_server_connection,
    struct ObpfLobbyUser const* user,
    char const* lobby_name,
    uint16_t lobby_size
);
SIMULATOR_EXPORT bool obpf_lobby_connection_destroy_lobby(
    struct ObpfLobbyServerConnection* lobby_server_connection,
    struct ObpfLobbyUser const* lobby_user,
    struct ObpfLobby** obpf_lobby
);
SIMULATOR_EXPORT bool obpf_lobby_connection_start_lobby(
    struct ObpfLobbyServerConnection* lobby_server_connection,
    struct ObpfLobbyUser const* lobby_user,
    struct ObpfLobby const* obpf_lobby,
    uint16_t* out_server_port
);

// clang-format on

#ifdef __cplusplus
}
#endif
