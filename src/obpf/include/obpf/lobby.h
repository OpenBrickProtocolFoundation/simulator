#pragma once

#include <obpf_export.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

    // clang-format off

// lobby-server-related functions
OBPF_EXPORT struct ObpfLobbyServerConnection* obpf_create_lobby_server_connection(char const* host, uint16_t port);
OBPF_EXPORT void obpf_destroy_lobby_server_connection(struct ObpfLobbyServerConnection const* lobby_server_connection);

// user-related functions
OBPF_EXPORT struct ObpfLobbyUser* obpf_lobby_connection_register_user(
    struct ObpfLobbyServerConnection* lobby_server_connection,
    const char* username,
    const char* password
);
OBPF_EXPORT struct ObpfLobbyUser* obpf_lobby_connection_authenticate_user(
    struct ObpfLobbyServerConnection* lobby_server_connection,
    const char* username,
    const char* password
);
OBPF_EXPORT void obpf_lobby_unregister_user(
    struct ObpfLobbyServerConnection* lobby_server_connection,
    struct ObpfLobbyUser** user_pointer
);
OBPF_EXPORT void obpf_user_destroy(struct ObpfLobbyUser const* user);

// lobby-related functions
OBPF_EXPORT struct ObpfLobby* obpf_lobby_connection_create_lobby(
    struct ObpfLobbyServerConnection* lobby_server_connection,
    struct ObpfLobbyUser const* user,
    char const* lobby_name,
    uint16_t lobby_size
);
OBPF_EXPORT bool obpf_lobby_connection_destroy_lobby(
    struct ObpfLobbyServerConnection* lobby_server_connection,
    struct ObpfLobbyUser const* lobby_user,
    struct ObpfLobby** obpf_lobby
);
OBPF_EXPORT bool obpf_lobby_connection_start_lobby(
    struct ObpfLobbyServerConnection* lobby_server_connection,
    struct ObpfLobbyUser const* lobby_user,
    struct ObpfLobby const* obpf_lobby,
    uint16_t* out_server_port
);

OBPF_EXPORT struct ObpfLobbyList const* obpf_get_lobby_list(
    struct ObpfLobbyServerConnection* lobby_server_connection
);

OBPF_EXPORT void obpf_free_lobby_list(
    struct ObpfLobbyList const* lobby_list
);

OBPF_EXPORT size_t obpf_lobby_list_length(
    struct ObpfLobbyList const* lobby_list
);

OBPF_EXPORT struct ObpfLobbyInfo const* obpf_lobby_list_at(
    struct ObpfLobbyList const* lobby_list,
    size_t index
);

OBPF_EXPORT char const* obpf_lobby_info_id(
    struct ObpfLobbyInfo const* lobby_info
);

OBPF_EXPORT char const* obpf_lobby_info_name(
    struct ObpfLobbyInfo const* lobby_info
);

OBPF_EXPORT uint16_t obpf_lobby_info_size(
    struct ObpfLobbyInfo const* lobby_info
);

OBPF_EXPORT uint16_t obpf_lobby_info_num_players_in_lobby(
    struct ObpfLobbyInfo const* lobby_info
);

OBPF_EXPORT char const* obpf_lobby_info_host_id(
    struct ObpfLobbyInfo const* lobby_info
);

OBPF_EXPORT char const* obpf_lobby_info_host_name(
    struct ObpfLobbyInfo const* lobby_info
);

OBPF_EXPORT struct ObpfLobby* obpf_lobby_connection_join(
    struct ObpfLobbyServerConnection* lobby_server_connection,
    struct ObpfLobbyInfo const* lobby_info,
    struct ObpfLobbyUser const* lobby_user
);

OBPF_EXPORT uint16_t obpf_lobby_set_ready(
    struct ObpfLobbyServerConnection* lobby_server_connection,
    struct ObpfLobbyUser const* lobby_user,
    struct ObpfLobby* obpf_lobby
);

OBPF_EXPORT struct ObpfLobbyDetails const* obpf_lobby_connection_get_lobby_details(
    struct ObpfLobbyServerConnection* lobby_server_connection,
    struct ObpfLobbyInfo const* lobby_info,
    struct ObpfLobbyUser const* lobby_user
);

OBPF_EXPORT void obpf_free_lobby_details(
    struct ObpfLobbyDetails const* lobby_details
);

OBPF_EXPORT char const * obpf_lobby_details_id(
    struct ObpfLobbyDetails const* lobby_details
);

OBPF_EXPORT char const * obpf_lobby_details_name(
    struct ObpfLobbyDetails const* lobby_details
);

OBPF_EXPORT uint16_t obpf_lobby_details_size(
    struct ObpfLobbyDetails const* lobby_details
);

OBPF_EXPORT size_t obpf_lobby_details_num_clients(
    struct ObpfLobbyDetails const* lobby_details
);

OBPF_EXPORT char const * obpf_lobby_details_client_id(
    struct ObpfLobbyDetails const* lobby_details,
    size_t index
);

OBPF_EXPORT char const * obpf_lobby_details_client_name(
    struct ObpfLobbyDetails const* lobby_details,
    size_t index
);

OBPF_EXPORT bool obpf_lobby_details_client_is_ready(
    struct ObpfLobbyDetails const* lobby_details,
    size_t index
);

OBPF_EXPORT char const * obpf_lobby_details_host_id(
    struct ObpfLobbyDetails const* lobby_details
);

OBPF_EXPORT char const * obpf_lobby_details_host_name(
    struct ObpfLobbyDetails const* lobby_details
);

// clang-format on

#ifdef __cplusplus
}
#endif
