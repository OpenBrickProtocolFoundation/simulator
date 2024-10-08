#pragma once

#include "lobby.hpp"
#include "user.hpp"
#include <cstdint>
#include <format>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <tl/expected.hpp>

struct LobbySettings final {
    std::string name;
    std::uint16_t size;

    LobbySettings(std::string name, std::uint16_t const size) : name{ std::move(name) }, size{ size } { }
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LobbySettings, name, size);

enum class LobbyCreationError {
    NotLoggedIn,
    AlreadyJoined,
    Unknown,
};

enum class GameStartError {
    NotLoggedIn,
    NotAllPlayersReady,
    LobbyNotFound,
    IsNotHost,
    AlreadyRunning,
    Unknown,
};

enum class LobbyDestructionError {
    NotLoggedIn,
    LobbyNotFound,
    IsNotHost,
    Unknown,
};

enum class LobbyJoinError {
    NotLoggedIn,
    LobbyNotFound,
    LobbyFullOrAlreadyJoined,
    Unknown,
};

enum class SetClientReadyError {
    NotLoggedIn,
    LobbyNotFoundOrClosed,
    NotInsideLobby,
    Unknown,
};

enum class LobbyDetailsError {
    NotLoggedIn,
    LobbyNotFoundOrClosed,
    Unknown,
};

enum class TcpPort : std::uint16_t {};

struct PlayerInfo final {
    std::string id;
    std::string name;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PlayerInfo, id, name);

struct LobbyInfo final {
    std::string id;
    std::string name;
    std::uint16_t size{};
    std::uint16_t num_players_in_lobby{};
    PlayerInfo host_info;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LobbyInfo, id, name, size, num_players_in_lobby, host_info);

struct LobbyList final {
    std::vector<LobbyInfo> lobbies;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LobbyList, lobbies);

struct ClientPlayerInfo final {
    std::string id;
    std::string name;
    bool is_ready{};
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ClientPlayerInfo, id, name, is_ready);

struct LobbyDetails final {
    std::string id;
    std::string name;
    std::uint16_t size{};
    std::vector<ClientPlayerInfo> client_infos;
    PlayerInfo host_info;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LobbyDetails, id, name, size, client_infos, host_info);

class LobbyServerConnection final {
private:
    std::string m_url;

    [[nodiscard]] std::string const& url() const {
        return m_url;
    }

    [[nodiscard]] std::string endpoint(std::string_view const path) const {
        if (path.starts_with('/')) {
            return std::format("{}{}", url(), path);
        }
        return std::format("{}/{}", url(), path);
    }

public:
    LobbyServerConnection(std::string_view const host, std::uint16_t const port) {
        if (host.ends_with('/')) {
            m_url = std::format("{}:{}", host.substr(0, host.length() - 1), port);
        } else {
            m_url = std::format("{}:{}", host, port);
        }
    }

    LobbyServerConnection(LobbyServerConnection const& other) = delete;
    LobbyServerConnection(LobbyServerConnection&& other) noexcept = delete;
    LobbyServerConnection& operator=(LobbyServerConnection const& other) = delete;
    LobbyServerConnection& operator=(LobbyServerConnection&& other) noexcept = delete;
    ~LobbyServerConnection() = default;

    [[nodiscard]] std::optional<User> register_user(std::string username, std::string password);
    [[nodiscard]] std::optional<User> authenticate(std::string username, std::string password);
    void unregister(User& user);
    [[nodiscard]] tl::expected<Lobby, LobbyCreationError> create_lobby(User const& user, LobbySettings const& settings);
    [[nodiscard]] tl::expected<TcpPort, GameStartError> start(User const& user, Lobby const& lobby);
    [[nodiscard]] LobbyList lobbies();
    [[nodiscard]] tl::expected<void, LobbyDestructionError> destroy_lobby(User const& user, Lobby&& lobby);
    [[nodiscard]] tl::expected<Lobby, LobbyJoinError> join(User const& user, LobbyInfo const& lobby_info);
    [[nodiscard]] tl::expected<TcpPort, SetClientReadyError> set_ready(User const& user, Lobby const& lobby);
    // clang-format off
    [[nodiscard]] tl::expected<LobbyDetails, LobbyDetailsError> lobby_details(
        User const& user,
        LobbyInfo const& lobby_info
    );
    // clang-format on
};
