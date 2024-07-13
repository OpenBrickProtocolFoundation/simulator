#include <spdlog/spdlog.h>
#include <cassert>
#include <gsl/gsl>
#include <simulator/tetrion.hpp>
#include <simulator/wallkicks.hpp>

void ObpfTetrion::simulate_up_until(std::uint64_t const frame) {
    while (m_next_frame <= frame) {
        process_events();
        if (m_next_frame == m_next_gravity_frame) {
            move_down();
            auto const gravity_delay =
                m_is_soft_dropping
                    ? std::max(
                          u64{ 1 },
                          static_cast<u64>(std::round(static_cast<double>(gravity_delay_by_level(level())) / 20.0))
                      )
                    : gravity_delay_by_level(level());
            m_next_gravity_frame += gravity_delay;
        }
        auto const auto_shift_movement = m_auto_shift_state.poll();
        if (auto_shift_movement == AutoShiftDirection::Left) {
            move_left();
        } else if (auto_shift_movement == AutoShiftDirection::Right) {
            move_right();
        }
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

    m_is_soft_dropping = false;
    m_next_gravity_frame = m_next_frame + gravity_delay_by_level(level());
}

void ObpfTetrion::process_events() {
    for (auto const& event : m_events) {
        assert(event.frame >= m_next_frame);
        if (event.frame == m_next_frame) {
            switch (event.type) {
                case EventType::Pressed:
                    handle_key_press(event.key);
                    break;
                case EventType::Released:
                    handle_key_release(event.key);
                    break;
            }
        }
    }
    std::erase_if(m_events, [this](auto const& event) { return event.frame <= m_next_frame; });
}

void ObpfTetrion::handle_key_press(Key const key) {
    switch (key) {
        case Key::Left:
            m_auto_shift_state.left_pressed();
            break;
        case Key::Right:
            m_auto_shift_state.right_pressed();
            break;
        case Key::Down:
            m_is_soft_dropping = true;
            m_next_gravity_frame = m_next_frame;
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

void ObpfTetrion::handle_key_release(Key const key) {
    switch (key) {
        case Key::Left:
            m_auto_shift_state.left_released();
            return;
        case Key::Right:
            m_auto_shift_state.right_released();
            return;
        case Key::Down:
            m_is_soft_dropping = false;
            m_next_gravity_frame = m_next_frame + gravity_delay_by_level(level());
            return;
        case Key::Drop:
        case Key::RotateClockwise:
        case Key::RotateCounterClockwise:
        case Key::Hold:
            // releasing these keys does nothing, so we can ignore them
            return;
    }
    std::unreachable();
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

void ObpfTetrion::rotate(RotationDirection const direction) {
    if (not m_active_tetromino.has_value()) {
        return;
    }
    auto const from_rotation = m_active_tetromino->rotation;
    auto const to_rotation = from_rotation + direction;
    m_active_tetromino->rotation = to_rotation;

    auto const& wall_kick_table = get_wall_kick_table(m_active_tetromino->type, from_rotation, to_rotation);
    for (auto const translation : wall_kick_table) {
        m_active_tetromino->position += translation;
        if (is_active_tetromino_position_valid()) {
            return;
        }
        m_active_tetromino->position -= translation;
    }

    m_active_tetromino->rotation = from_rotation;
}

void ObpfTetrion::rotate_clockwise() {
    rotate(RotationDirection::Clockwise);
}

void ObpfTetrion::rotate_counter_clockwise() {
    rotate(RotationDirection::CounterClockwise);
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

        auto const old_level = level();
        ++m_lines_cleared;
        if (level() > old_level) {
            spdlog::info("changed to level {}", level());
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
