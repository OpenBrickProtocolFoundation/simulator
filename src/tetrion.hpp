#pragma once

#include "matrix.hpp"
#include "tetromino.hpp"
#include <cstdint>
#include <optional>

struct Tetrion final {
private:
    Matrix m_matrix;
    std::optional<Tetromino> m_active_tetromino;
    std::uint64_t m_current_frame = 0;

public:
    Tetrion() {
        spawn_next_tetromino();
    }

    [[nodiscard]] Matrix const& matrix() const {
        return m_matrix;
    }

    [[nodiscard]] std::optional<Tetromino> active_tetromino() const {
        return m_active_tetromino;
    }

    void simulate_up_until(std::uint64_t frame);

private:
    void freeze_and_destroy_active_tetromino();
    [[nodiscard]] bool is_active_tetromino_position_valid() const;
    void spawn_next_tetromino();
};
