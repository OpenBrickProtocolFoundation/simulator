#include "tetrion.hpp"

void Tetrion::simulate_up_until(std::uint64_t const frame) {
    while (m_current_frame < frame) {
        if (m_active_tetromino.has_value()) {
            m_active_tetromino.value().position.y += 1;
            if (not is_active_tetromino_position_valid()) {
                m_active_tetromino.value().position.y -= 1;
                freeze_and_destroy_active_tetromino();
                spawn_next_tetromino();
            }
        }
        ++m_current_frame;
    }
}

void Tetrion::freeze_and_destroy_active_tetromino() {
    if (not active_tetromino().has_value()) {
        return;
    }
    auto const mino_positions = get_mino_positions(active_tetromino().value());
    for (auto const position : mino_positions) {
        m_matrix[position] = active_tetromino().value().type;
    }
    m_active_tetromino = std::nullopt;
}

bool Tetrion::is_active_tetromino_position_valid() const {
    if (not active_tetromino().has_value()) {
        return true;
    }
    auto const mino_positions = get_mino_positions(active_tetromino().value());
    for (auto const position : mino_positions) {
        if (position.y >= Matrix::height or m_matrix[position] != TetrominoType::Empty) {
            return false;
        }
    }
    return true;
}

void Tetrion::spawn_next_tetromino() {
    m_active_tetromino = Tetromino{
        Vec2{ 3, 0 },
        Rotation::North,
        TetrominoType::O
    };
}
