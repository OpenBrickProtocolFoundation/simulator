#include <cassert>
#include <gsl/gsl>
#include <simulator/tetrion.hpp>

void ObpfTetrion::simulate_up_until(std::uint64_t const frame) {
    while (m_next_frame <= frame) {
        if (m_next_frame % 28 == 0) {
            move_down();
        }
        process_events();
        clear_lines();
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
        if (position.x < 0 or position.x >= Matrix::width or position.y >= Matrix::height
            or m_matrix[position] != TetrominoType::Empty) {
            return false;
        }
    }
    return true;
}

void ObpfTetrion::spawn_next_tetromino() {
    auto const next_type = m_bags.at(0).tetrominos.at(m_bag_index);
    if (m_bag_index == std::tuple_size_v<decltype(Bag::tetrominos)> - 1) {
        m_bag_index = 0;
        m_bags.at(0) = m_bags.at(1);
        m_bags.at(1) = Bag{ m_random };
    } else {
        ++m_bag_index;
    }
    m_active_tetromino = Tetromino{ spawn_position, spawn_rotation, next_type };
}

void ObpfTetrion::process_events() {
    for (auto const& event : m_events) {
        assert(event.frame >= m_next_frame);
        if (event.frame == m_next_frame) {
            switch (event.type) {
                case EventType::Pressed:
                    handle_keypress(event.key);
                    break;
                case EventType::Released:
                    break;
            }
        }
    }
    std::erase_if(m_events, [this](auto const& event) { return event.frame <= m_next_frame; });
}

void ObpfTetrion::handle_keypress(Key const key) {
    switch (key) {
        case Key::Left:
            move_left();
            break;
        case Key::Right:
            move_right();
            break;
        case Key::Down:
            move_down();
            break;
        case Key::Drop:
            drop();
            break;
        case Key::RotateClockwise:
            rotate_clockwise();
            break;
        case Key::RotateCounterClockwise:
            rotate_counter_clockwise();
            break;
        case Key::Hold:
            // todo
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

void ObpfTetrion::rotate_clockwise() {
    if (m_active_tetromino.has_value()) {
        --m_active_tetromino.value().rotation;
        if (not is_active_tetromino_position_valid()) {
            ++m_active_tetromino.value().rotation;
        }
    }
}

void ObpfTetrion::rotate_counter_clockwise() {
    if (m_active_tetromino.has_value()) {
        ++m_active_tetromino.value().rotation;
        if (not is_active_tetromino_position_valid()) {
            --m_active_tetromino.value().rotation;
        }
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

void ObpfTetrion::clear_lines() {
    for (auto i = std::size_t{ 0 }; i < Matrix::height;) {
        auto const line = Matrix::height - i - 1;
        if (not m_matrix.is_line_full(line)) {
            ++i;
            continue;
        }

        for (auto destination_line = line; destination_line > 0; --destination_line) {
            m_matrix.copy_line(destination_line, destination_line - 1);
        }
        m_matrix.fill(0, TetrominoType::Empty);
    }
}

[[nodiscard]] std::array<Bag, 2> ObpfTetrion::create_two_bags(c2k::Random& random) {
    auto const bag0 = Bag{ random };
    auto const bag1 = Bag{ random };
    return std::array{ bag0, bag1 };
}
