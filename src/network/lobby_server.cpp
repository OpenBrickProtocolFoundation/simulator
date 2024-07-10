#include <spdlog/spdlog.h>
#include <crapper/crapper.hpp>
#include <iostream>
#include <network/lobby_server.hpp>

struct Credentials final {
    std::string username;
    std::string password;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Credentials, username, password);

struct LoginResponse final {
    std::string jwt;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LoginResponse, jwt);

struct LobbyCreationResponse final {
    std::string id;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LobbyCreationResponse, id);

struct StartResponse final {
    std::uint16_t port;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(StartResponse, port);

struct SetClientReadyResponse final {
    std::uint16_t port;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SetClientReadyResponse, port);

struct HostInfo final {
    std::string id;
    std::string name;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(HostInfo, id, name);

struct LobbyDetailsResponse final {
    std::string name;
    std::uint16_t size{};
    HostInfo host_info;
    std::vector<ClientPlayerInfo> player_infos;
    std::optional<std::uint16_t> gameserver_port;
};

void to_json(nlohmann::json& json, LobbyDetailsResponse const& response) {
    json = nlohmann::json{
        {         "name",         response.name },
        {         "size",         response.size },
        {    "host_info",    response.host_info },
        { "player_infos", response.player_infos },
    };
    if (response.gameserver_port.has_value()) {
        json["gameserver_port"] = response.gameserver_port.value();
    }
}

void from_json(nlohmann::json const& json, LobbyDetailsResponse& response) {
    json.at("name").get_to(response.name);
    json.at("size").get_to(response.size);
    json.at("host_info").get_to(response.host_info);
    json.at("player_infos").get_to(response.player_infos);
    if (json.contains("gameserver_port")) {
        response.gameserver_port = std::uint16_t{};
        json.at("gameserver_port").get_to(response.gameserver_port.value());
    } else {
        response.gameserver_port = std::nullopt;
    }
}

template<typename Deserialized>
[[nodiscard]] static std::optional<Deserialized> deserialize(Response const& response) {
    try {
        return nlohmann::json::parse(response.body()).get<Deserialized>();
    } catch (nlohmann::json::parse_error const& exception) {
        spdlog::error("invalid JSON in response to HTTP request: {}", exception.what());
    } catch (nlohmann::json::out_of_range const& exception) {
        spdlog::error("unable to deserialize response to HTTP request: {}", exception.what());
    } catch (std::exception const& exception) {
        spdlog::error("unexpected error: {}", exception.what());
    } catch (...) {
        spdlog::error("unexpected error");
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<User> LobbyServerConnection::register_user(
    std::string username,
    std::string password
) {
    auto const response =
        Crapper{}.post(endpoint("register")).json(Credentials{ username, password }).send();
    if (response.status() != HttpStatusCode::NoContent) {
        return std::nullopt;
    }
    return authenticate(std::move(username), std::move(password));
}

[[nodiscard]] std::optional<User> LobbyServerConnection::authenticate(
    std::string username,
    std::string password
) {
    auto const response = Crapper{}
                              .post(endpoint("login"))
                              .json(Credentials{ std::move(username), std::move(password) })
                              .send();
    if (response.status() != HttpStatusCode::Ok) {
        return std::nullopt;
    }
    auto login_response = deserialize<LoginResponse>(response);
    if (not login_response.has_value()) {
        return std::nullopt;
    }
    return User{ std::move(login_response).value().jwt };
}

void LobbyServerConnection::unregister(User& user) {
    std::ignore = Crapper{}
                      .post(endpoint("unregister"))
                      .header(HeaderKey::Authorization, std::format("Bearer {}", user.m_token))
                      .send();
    user.m_token.clear();
}

[[nodiscard]] tl::expected<Lobby, LobbyCreationError> LobbyServerConnection::create_lobby(
    User const& user,
    LobbySettings const& settings
) {
    if (not user.is_logged_in()) {
        return tl::unexpected{ LobbyCreationError::NotLoggedIn };
    }
    auto const response =
        Crapper{}
            .post(endpoint("lobbies"))
            .header(HeaderKey::Authorization, std::format("Bearer {}", user.auth_token()))
            .json(settings)
            .send();
    if (response.status() == HttpStatusCode::BadRequest) {
        return tl::unexpected{ LobbyCreationError::AlreadyJoined };
    }
    if (response.status() != HttpStatusCode::Created) {
        return tl::unexpected{ LobbyCreationError::Unknown };
    }
    auto lobby_creation_response = deserialize<LobbyCreationResponse>(response);
    if (not lobby_creation_response.has_value()) {
        return tl::unexpected{ LobbyCreationError::Unknown };
    }
    return Lobby{ std::move(lobby_creation_response).value().id };
}

[[nodiscard]] tl::expected<TcpPort, GameStartError> LobbyServerConnection::start(
    User const& user,
    Lobby const& lobby
) {
    if (not user.is_logged_in()) {
        return tl::unexpected{ GameStartError::NotLoggedIn };
    }
    auto const response =
        Crapper{}
            .post(endpoint(std::format("lobbies/{}/start", lobby.id)))
            .header(HeaderKey::Authorization, std::format("Bearer {}", user.auth_token()))
            .send();
    switch (response.status()) {
        case HttpStatusCode::NotFound:
            return tl::unexpected{ GameStartError::LobbyNotFound };
        case HttpStatusCode::Forbidden:
            return tl::unexpected{ GameStartError::IsNotHost };
        case HttpStatusCode::BadRequest:
            return tl::unexpected{ GameStartError::AlreadyRunning };
        case HttpStatusCode::TooEarly:
            return tl::unexpected{ GameStartError::NotAllPlayersReady };
        default:
            if (response.status() != HttpStatusCode::Ok) {
                return tl::unexpected{ GameStartError::Unknown };
            }
    }
    auto const start_response = deserialize<StartResponse>(response);
    if (not start_response.has_value()) {
        return tl::unexpected{ GameStartError::Unknown };
    }
    return TcpPort{ start_response.value().port };
}

[[nodiscard]] LobbyList LobbyServerConnection::lobbies() {
    auto const response = Crapper{}.get(endpoint("lobbies")).send();
    if (response.status() != HttpStatusCode::Ok) {
        throw std::runtime_error{ "internal lobby error" };
    }

    return nlohmann::json::parse(response.body()).get<LobbyList>();
}

[[nodiscard]] tl::expected<void, LobbyDestructionError> LobbyServerConnection::destroy_lobby(
    User const& user,
    Lobby&& lobby
) {
    if (not user.is_logged_in()) {
        return tl::unexpected{ LobbyDestructionError::NotLoggedIn };
    }
    auto const response =
        Crapper{}
            .delete_(endpoint(std::format("lobbies/{}", lobby.id)))
            .header(HeaderKey::Authorization, std::format("Bearer {}", user.auth_token()))
            .send();
    switch (response.status()) {
        case HttpStatusCode::NoContent:
            return {};
        case HttpStatusCode::NotFound:
            return tl::unexpected{ LobbyDestructionError::LobbyNotFound };
        case HttpStatusCode::Forbidden:
            return tl::unexpected{ LobbyDestructionError::IsNotHost };
        default:
            return tl::unexpected{ LobbyDestructionError::Unknown };
    }
}

[[nodiscard]] tl::expected<Lobby, LobbyJoinError> LobbyServerConnection::join(
    User const& user,
    LobbyInfo const& lobby_info
) {
    std::cerr << "checking if user is logged in...\n";
    std::cerr << &user << '\n';
    std::cerr << user.auth_token() << '\n';
    if (not user.is_logged_in()) {
        return tl::unexpected{ LobbyJoinError::NotLoggedIn };
    }
    std::cerr << "user is logged in\n";
    std::cerr << "lobby id is: " << lobby_info.id << '\n';
    auto const response =
        Crapper{}
            .post(endpoint(std::format("lobbies/{}", lobby_info.id)))
            .header(HeaderKey::Authorization, std::format("Bearer {}", user.auth_token()))
            .send();
    std::cerr << "received response, status: " << std::to_underlying(response.status()) << '\n';
    if (response.status() == HttpStatusCode::NotFound) {
        return tl::unexpected{ LobbyJoinError::LobbyNotFound };
    }
    if (response.status() == HttpStatusCode::BadRequest) {
        return tl::unexpected{ LobbyJoinError::LobbyFullOrAlreadyJoined };
    }
    if (response.status() == HttpStatusCode::NoContent) {
        return Lobby{ lobby_info.id };
    }
    return tl::unexpected{ LobbyJoinError::Unknown };
}

[[nodiscard]] tl::expected<TcpPort, SetClientReadyError> LobbyServerConnection::set_ready(
    User const& user,
    Lobby const& lobby
) {
    if (not user.is_logged_in()) {
        return tl::unexpected{ SetClientReadyError::NotLoggedIn };
    }
    auto const response =
        Crapper{}
            .post(endpoint(std::format("lobby/{}/ready", lobby.id)))
            .header(HeaderKey::Authorization, std::format("Bearer {}", user.auth_token()))
            .send();
    if (response.status() == HttpStatusCode::NotFound) {
        return tl::unexpected{ SetClientReadyError::LobbyNotFoundOrClosed };
    }

    if (response.status() == HttpStatusCode::Forbidden) {
        return tl::unexpected{ SetClientReadyError::NotInsideLobby };
    }

    if (response.status() == HttpStatusCode::Ok) {
        return TcpPort{ nlohmann::json::parse(response.body()).get<SetClientReadyResponse>().port };
    }

    return tl::unexpected{ SetClientReadyError::Unknown };
}

tl::expected<LobbyDetails, LobbyDetailsError> LobbyServerConnection::lobby_details(
    User const& user,
    LobbyInfo const& lobby_info
) {
    if (not user.is_logged_in()) {
        return tl::unexpected{ LobbyDetailsError::NotLoggedIn };
    }
    auto const response =
        Crapper{}
            .get(endpoint(std::format("lobbies/{}", lobby_info.id)))
            .header(HeaderKey::Authorization, std::format("Bearer {}", user.auth_token()))
            .send();
    if (response.status() == HttpStatusCode::NotFound) {
        return tl::unexpected{ LobbyDetailsError::LobbyNotFoundOrClosed };
    }

    std::cerr << response.body() << '\n';
    auto data = nlohmann::json::parse(response.body()).get<LobbyDetailsResponse>();
    return LobbyDetails{
        lobby_info.id,
        std::move(data.name),
        data.size,
        std::move(data.player_infos),
        PlayerInfo{
                   std::move(data.host_info.id),
                   std::move(data.host_info.name),
                   },
    };
}
