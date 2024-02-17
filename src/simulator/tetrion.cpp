#include "tetrion.hpp"


#include <cassert>

void ObpfTetrion::simulate_up_until(std::uint64_t const frame) {
    while (m_next_frame <= frame) {
        if (m_next_frame % 28 == 0) {
            move_down();
        }
        process_events();
        ++m_next_frame;
    }
}

void ObpfTetrion::enqueue_event(Event const& event) {
    m_events.push_back(event);
}

void ObpfTetrion::freeze_and_destroy_active_tetromino() {
    if (not active_tetromino().has_value()) {
        return;
    }
    auto const mino_positions = get_mino_positions(active_tetromino().value());
    for (auto const position : mino_positions) {
        m_matrix[position] = active_tetromino().value().type;
    }
    m_active_tetromino = std::nullopt;
}

bool ObpfTetrion::is_active_tetromino_position_valid() const {
    if (not active_tetromino().has_value()) {
        return true;
    }
    auto const mino_positions = get_mino_positions(active_tetromino().value());
    for (auto const position : mino_positions) {
        if (position.x < 0 or position.x >= ObpfMatrix::width or position.y >= ObpfMatrix::height
            or m_matrix[position] != TetrominoType::Empty) {
            return false;
        }
    }
    return true;
}

void ObpfTetrion::spawn_next_tetromino() {
    m_active_tetromino = Tetromino{ spawn_position, spawn_rotation, TetrominoType::O };
}

void ObpfTetrion::process_events() {
    for (auto const& event : m_events) {
        assert(event.frame >= m_next_frame);
        if (event.frame == m_next_frame) {
            switch (event.type) {
                case OBPF_PRESSED:
                    handle_keypress(event.key);
                    break;
                case OBPF_RELEASED:
                    break;
            }
        }
    }
    std::erase_if(m_events, [this](auto const& event) { return event.frame <= m_next_frame; });
}

void ObpfTetrion::handle_keypress(ObpfKey const key) {
    switch (key) {
        case OBPF_KEY_LEFT:
            move_left();
            break;
        case OBPF_KEY_RIGHT:
            move_right();
            break;
        case OBPF_KEY_DROP:
            drop();
            break;
    }
}

void ObpfTetrion::move_left() {
    if (not active_tetromino().has_value()) {
        return;
    }
    --m_active_tetromino.value().position.x;
    if (not is_active_tetromino_position_valid()) {
        ++m_active_tetromino.value().position.x;
    }
}

void ObpfTetrion::move_right() {
    if (not active_tetromino().has_value()) {
        return;
    }
    ++m_active_tetromino.value().position.x;
    if (not is_active_tetromino_position_valid()) {
        --m_active_tetromino.value().position.x;
    }
}

void ObpfTetrion::move_down() {
    if (not active_tetromino().has_value()) {
        return;
    }
    ++m_active_tetromino.value().position.y;
    if (not is_active_tetromino_position_valid()) {
        --m_active_tetromino.value().position.y;
        freeze_and_destroy_active_tetromino();
        spawn_next_tetromino();
    }
}

void ObpfTetrion::drop() {
    if (not active_tetromino().has_value()) {
        return;
    }
    do {
        ++m_active_tetromino.value().position.y;
    } while (is_active_tetromino_position_valid());
    --m_active_tetromino.value().position.y;
    freeze_and_destroy_active_tetromino();
    spawn_next_tetromino();
}
