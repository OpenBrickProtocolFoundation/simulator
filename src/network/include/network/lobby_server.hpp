#pragma once

#include "lobby.hpp"
#include "user.hpp"
#include <cstdint>
#include <format>
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

enum class TcpPort : std::uint16_t {};

class LobbyServer final {
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
    LobbyServer(std::string_view const host, std::uint16_t const port) {
        if (host.ends_with('/')) {
            m_url = std::format("{}:{}", host.substr(0, host.length() - 1), port);
        } else {
            m_url = std::format("{}:{}", host, port);
        }
    }

    LobbyServer(LobbyServer const& other) = delete;
    LobbyServer(LobbyServer&& other) noexcept = delete;
    LobbyServer& operator=(LobbyServer const& other) = delete;
    LobbyServer& operator=(LobbyServer&& other) noexcept = delete;
    ~LobbyServer() = default;

    [[nodiscard]] std::optional<User> register_user(std::string username, std::string password);
    [[nodiscard]] std::optional<User> authenticate(std::string username, std::string password);
    void unregister(User& user);
    [[nodiscard]] tl::expected<Lobby, LobbyCreationError> create_lobby(User const& user, LobbySettings const& settings);
    [[nodiscard]] tl::expected<TcpPort, GameStartError> start(User const& user, Lobby const& lobby);
};
