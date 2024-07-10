#pragma once

#include <stdexcept>
#include <string>

class User final {
    friend class LobbyServerConnection;

private:
    std::string m_token;

    explicit User(std::string token)
        : m_token{ std::move(token) } {}

    [[nodiscard]] std::string const& auth_token() const {
        if (not is_logged_in()) {
            throw std::logic_error{ "unable to get auth token from logged-out user" };
        }
        return m_token;
    }

public:
    [[nodiscard]] bool is_logged_in() const {
        return not m_token.empty();
    }
};
