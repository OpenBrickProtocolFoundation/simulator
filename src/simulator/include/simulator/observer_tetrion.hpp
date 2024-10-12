#pragma once

#include "tetrion.hpp"

struct ObserverTetrion final : ObpfTetrion {
    friend struct MultiplayerTetrion;

private:
    struct Key {};

    u8 m_client_id;

public:
    ObserverTetrion(u64 const seed, u64 const start_frame, u8 const m_client_id, Key)
        : ObpfTetrion{ seed, start_frame }, m_client_id{ m_client_id } {}

    void simulate_next_frame(KeyState) override {}

    [[nodiscard]] u8 client_id() const {
        return m_client_id;
    }

    [[nodiscard]] bool is_observer() const override {
        return true;
    }

private:
    void process_key_state(KeyState key_state);
};
