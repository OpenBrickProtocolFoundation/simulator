#include <spdlog/spdlog.h>
#include <cassert>
#include <gsl/gsl>
#include <iostream>
#include <lib2k/static_vector.hpp>
#include <ranges>
#include <simulator/tetrion.hpp>
#include <simulator/wallkicks.hpp>

void ObpfTetrion::simulate_up_until(std::uint64_t const frame) {
    while (m_next_frame <= frame) {
        auto const line_clear_delay_poll_result = m_line_clear_delay.poll();

        if (std::get_if<LineClearDelay::DelayEnded>(&line_clear_delay_poll_result) != nullptr) {
            auto const lines = std::get<LineClearDelay::DelayEnded>(line_clear_delay_poll_result).lines;
            clear_lines(lines);
        } else if (std::get_if<LineClearDelay::DelayIsActive>(&line_clear_delay_poll_result) != nullptr) {
            process_events();
            ++m_next_frame;
            continue;
        } else {
            assert(std::holds_alternative<LineClearDelay::DelayIsInactive>(line_clear_delay_poll_result));
        }

        switch (m_entry_delay.poll()) {
            case EntryDelayPollResult::ShouldSpawn:
                spawn_next_tetromino();
                break;
            case EntryDelayPollResult::ShouldNotSpawn:
                break;
        }

        process_events();

        if (m_next_frame == m_next_gravity_frame) {
            move_down(m_is_soft_dropping ? DownMovementType::SoftDrop : DownMovementType::Gravity);
            auto const gravity_delay =
                m_is_soft_dropping
                    ? std::max(
                          u64{ 1 },
                          static_cast<u64>(std::round(static_cast<double>(gravity_delay_by_level(level())) / 20.0))
                      )
                    : gravity_delay_by_level(level());
            m_next_gravity_frame += gravity_delay;
        }

        switch (m_lock_delay_state.poll()) {
            case LockDelayPollResult::ShouldLock:
                freeze_and_destroy_active_tetromino();
                m_entry_delay.start();
                break;
            case LockDelayPollResult::ShouldNotLock:
                break;
        }

        switch (m_auto_shift_state.poll()) {
            case AutoShiftDirection::Left:
                move_left();
                break;
            case AutoShiftDirection::Right:
                move_right();
                break;
            case AutoShiftDirection::None:
                break;
        }
        determine_lines_to_clear();

        refresh_ghost_tetromino();

        ++m_next_frame;
    }
}

void ObpfTetrion::enqueue_event(Event const& event) {
    m_events.push_back(event);
}

void ObpfTetrion::set_lines_cleared_callback(CFacingOnLinesClearedCallback const callback) {
    spdlog::debug("lines cleared callback set");
    m_on_lines_cleared_callback = callback;
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

[[nodiscard]] bool ObpfTetrion::is_tetromino_position_valid(Tetromino const& tetromino) const {
    auto const mino_positions = get_mino_positions(tetromino);
    for (auto const position : mino_positions) {
        if (position.x < 0 or position.x >= Matrix::width or position.y >= Matrix::height
            or m_matrix[position] != TetrominoType::Empty) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool ObpfTetrion::is_active_tetromino_position_valid() const {
    return not active_tetromino().has_value() or is_tetromino_position_valid(active_tetromino().value());
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
    discard_events(m_next_frame);
}

void ObpfTetrion::discard_events(u64 const frame) {
    std::erase_if(m_events, [&](auto const& event) { return event.frame <= frame; });
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
    if (is_active_tetromino_position_valid()) {
        m_lock_delay_state.on_tetromino_moved(LockDelayMovementType::NotMovedDown);
    } else {
        ++m_active_tetromino.value().position.x;
    }
}

void ObpfTetrion::move_right() {
    if (not active_tetromino().has_value()) {
        return;
    }
    ++m_active_tetromino.value().position.x;
    if (is_active_tetromino_position_valid()) {
        m_lock_delay_state.on_tetromino_moved(LockDelayMovementType::NotMovedDown);
    } else {
        --m_active_tetromino.value().position.x;
    }
}

void ObpfTetrion::move_down(DownMovementType const movement_type) {
    if (not active_tetromino().has_value()) {
        return;
    }
    ++m_active_tetromino.value().position.y;
    if (is_active_tetromino_position_valid()) {
        m_lock_delay_state.on_tetromino_moved(LockDelayMovementType::MovedDown);
    } else {
        --m_active_tetromino.value().position.y;
        switch (movement_type) {
            case DownMovementType::Gravity:
                m_lock_delay_state.on_gravity_lock();
                break;
            case DownMovementType::SoftDrop:
                m_lock_delay_state.on_soft_drop_lock();
                m_is_soft_dropping = false;
                break;
        }
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
            m_lock_delay_state.on_tetromino_moved(LockDelayMovementType::NotMovedDown);
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
    m_lock_delay_state.on_hard_drop_lock();
}

void ObpfTetrion::determine_lines_to_clear() {
    auto lines_to_clear = c2k::StaticVector<u8, 4>{};
    for (auto i = std::size_t{ 0 }; i < Matrix::height; ++i) {
        auto const line = Matrix::height - i - 1;
        if (not m_matrix.is_line_full(line)) {
            continue;
        }

        std::cerr << "pushing back line to clear, line: " << line
                  << ", lines_to_clear.size(): " << lines_to_clear.size() << '\n';
        lines_to_clear.push_back(gsl::narrow<u8>(line));
    }

    if (not lines_to_clear.empty()) {
        std::cerr << "lines to clear: ";
        for (auto const line : lines_to_clear) {
            std::cerr << static_cast<int>(line) << ", ";
        }
        std::cerr << '\n';
        m_line_clear_delay.start(lines_to_clear);
        on_lines_cleared(lines_to_clear, LineClearDelay::delay);
    }
}

void ObpfTetrion::clear_lines(c2k::StaticVector<u8, 4> const lines) {
    auto num_lines_cleared = decltype(lines.front()){ 0 };
    for (auto const line_to_clear : lines) {
        std::cerr << "should clear line: " << static_cast<int>(line_to_clear) << '\n';
        for (auto i = decltype(line_to_clear){ 0 }; i < line_to_clear; ++i) {
            auto const line = line_to_clear - i + num_lines_cleared;
            std::cerr << "copying line " << line - 1 << " to line " << line << '\n';
            m_matrix.copy_line(line, line - 1);
        }
        ++num_lines_cleared;
        m_matrix.fill(num_lines_cleared, TetrominoType::Empty);
    }
    m_lines_cleared += gsl::narrow<decltype(m_lines_cleared)>(lines.size());
}

[[nodiscard]] u32 ObpfTetrion::level() const {
    return m_lines_cleared / 10;
}

void ObpfTetrion::refresh_ghost_tetromino() {
    if (not m_active_tetromino.has_value()) {
        m_ghost_tetromino = std::nullopt;
        return;
    }

    m_ghost_tetromino = m_active_tetromino;
    while (true) {
        ++m_ghost_tetromino->position.y;
        if (not is_tetromino_position_valid(m_ghost_tetromino.value())) {
            --m_ghost_tetromino->position.y;
            return;
        }
    }
}

void ObpfTetrion::on_lines_cleared(c2k::StaticVector<u8, 4> lines, u64 const delay) {
    if (m_on_lines_cleared_callback != nullptr) {
        assert(lines.capacity() == 4);
        auto const original_size = lines.size();
        while (lines.size() < decltype(lines)::capacity()) {
            lines.push_back(0);
        }
        spdlog::error(
            "invoking callback, lines cleared: {}, {}, {}, {}, delay: {}",
            lines.at(0),
            lines.at(1),
            lines.at(2),
            lines.at(3),
            delay
        );
        spdlog::error("callback address: {}", reinterpret_cast<void*>(m_on_lines_cleared_callback));
        m_on_lines_cleared_callback(
            gsl::narrow<u8>(original_size),
            lines.at(0),
            lines.at(1),
            lines.at(2),
            lines.at(3),
            delay
        );
    }
}

[[nodiscard]] std::array<Bag, 2> ObpfTetrion::create_two_bags(c2k::Random& random) {
    auto const bag0 = Bag{ random };
    auto const bag1 = Bag{ random };
    return std::array{ bag0, bag1 };
}
