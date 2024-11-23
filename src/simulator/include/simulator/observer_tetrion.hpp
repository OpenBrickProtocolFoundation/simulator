#pragma once

#include "tetrion.hpp"

struct ObserverTetrion final : ObpfTetrion {
    friend struct MultiplayerTetrion;

private:
    struct Key {};

    u8 m_client_id;
    bool m_is_connected = true;

public:
    explicit ObserverTetrion(u64 seed, u64 start_frame, u8 client_id, std::string player_name, Key);

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
