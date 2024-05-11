#include <crapper/crapper.hpp>
#include <network/lobby_server.hpp>
#include <spdlog/spdlog.h>

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

[[nodiscard]] std::optional<User> LobbyServer::register_user(std::string username, std::string password) {
    auto const response = Crapper{}.post(endpoint("register")).json(Credentials{ username, password }).send();
    if (response.status() != HttpStatusCode::NoContent) {
        return std::nullopt;
    }
    return authenticate(std::move(username), std::move(password));
}

[[nodiscard]] std::optional<User> LobbyServer::authenticate(std::string username, std::string password) {
    auto const response =
            Crapper{}.post(endpoint("login")).json(Credentials{ std::move(username), std::move(password) }).send();
    if (response.status() != HttpStatusCode::Ok) {
        return std::nullopt;
    }
    auto login_response = deserialize<LoginResponse>(response);
    if (not login_response.has_value()) {
        return std::nullopt;
    }
    return User{ std::move(login_response).value().jwt };
}

void LobbyServer::unregister(User& user) {
    std::ignore = Crapper{}
                          .post(endpoint("unregister"))
                          .header(HeaderKey::Authorization, std::format("Bearer {}", user.m_token))
                          .send();
    user.m_token.clear();
}

// clang-format off
[[nodiscard]] tl::expected<Lobby, LobbyCreationError> LobbyServer::create_lobby(
    User const& user,
    LobbySettings const& settings
) {
    // clang-format on
    if (not user.is_logged_in()) {
        return tl::unexpected{ LobbyCreationError::NotLoggedIn };
    }
    auto const response = Crapper{}
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

[[nodiscard]] tl::expected<TcpPort, GameStartError> LobbyServer::start(User const& user, Lobby const& lobby) {
    if (not user.is_logged_in()) {
        return tl::unexpected{ GameStartError::NotLoggedIn };
    }
    auto const response = Crapper{}
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
        // todo: handle case that not all players are ready
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
