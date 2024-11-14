#pragma once

#include "tetrion.hpp"

struct ObserverTetrion final : ObpfTetrion {
    friend struct MultiplayerTetrion;

private:
    struct Key {};

    u8 m_client_id;
    bool m_is_connected = true;

public:
    ObserverTetrion(u64 const seed, u64 const start_frame, u8 const m_client_id, std::string player_name, Key)
        : ObpfTetrion{ seed, start_frame, std::move(player_name) }, m_client_id{ m_client_id } {}

    [[nodiscard]] std::optional<GarbageSendEvent> simulate_next_frame(KeyState) override {
        return std::nullopt;
    }

    [[nodiscard]] u8 id() const override {
        return m_client_id;
    }

    [[nodiscard]] bool is_observer() const override {
        return true;
    }

    [[nodiscard]] bool is_connected() const override {
        return m_is_connected;
    }

private:
    [[nodiscard]] std::optional<GarbageSendEvent> process_key_state(KeyState key_state);
};
