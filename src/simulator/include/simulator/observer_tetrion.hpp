#pragma once

#include "tetrion.hpp"

struct ObserverTetrion final : ObpfTetrion {
    friend struct MultiplayerTetrion;

private:
    struct Key {};

    std::uint8_t m_client_id;

public:
    ObserverTetrion(std::uint64_t const seed, std::uint8_t const m_client_id, Key)
        : ObpfTetrion{ seed }, m_client_id{ m_client_id } {}

    void simulate_next_frame(KeyState) override {}

    [[nodiscard]] std::uint8_t client_id() const {
        return m_client_id;
    }

private:
    void process_key_state(KeyState key_state);
};
