#pragma once

#include <string>

class Lobby final {
    friend class LobbyServerConnection;

private:
    std::string id;

    explicit Lobby(std::string id) : id{ std::move(id) } { }
};
