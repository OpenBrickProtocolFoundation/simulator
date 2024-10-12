#include <spdlog/spdlog.h>
#include <cassert>
#include <gsl/gsl>
#include <lib2k/static_vector.hpp>
#include <lib2k/types.hpp>
#include <magic_enum.hpp>
#include <ranges>
#include <simulator/tetrion.hpp>
#include <simulator/wallkicks.hpp>

[[nodiscard]] static auto determine_pressed_keys(KeyState const previous_state, KeyState const current_state) {
    auto result = std::array<bool, magic_enum::enum_count<Key>()>{};
    for (auto const key : magic_enum::enum_values<Key>()) {
        auto const key_index = gsl::narrow<usize>(std::to_underlying(key));
        result.at(key_index) = current_state.get(key) and not previous_state.get(key);
    }
    return result;
}

[[nodiscard]] static auto determine_released_keys(KeyState const previous_state, KeyState const current_state) {
    auto result = std::array<bool, magic_enum::enum_count<Key>()>{};
    for (auto const key : magic_enum::enum_values<Key>()) {
        auto const key_index = gsl::narrow<usize>(std::to_underlying(key));
        result.at(key_index) = not current_state.get(key) and previous_state.get(key);
        if (result.at(key_index)) {
            spdlog::info("key {} released", magic_enum::enum_name(key));
        }
    }
    return result;
}

void ObpfTetrion::simulate_next_frame(KeyState const key_state) {
    if (is_game_over() or m_next_frame < m_start_frame) {
        ++m_next_frame;
        return;
    }
    if (m_next_frame == m_start_frame) {
        spawn_next_tetromino();
    }

    if (auto const line_clear_delay_poll_result = m_line_clear_delay.poll();
        std::get_if<LineClearDelay::DelayEnded>(&line_clear_delay_poll_result) != nullptr) {
        auto const lines = std::get<LineClearDelay::DelayEnded>(line_clear_delay_poll_result).lines;
        clear_lines(lines);
    } else if (std::get_if<LineClearDelay::DelayIsActive>(&line_clear_delay_poll_result) != nullptr) {
        process_keys(key_state);
        ++m_next_frame;
        return;
    } else {
        assert(std::holds_alternative<LineClearDelay::DelayIsInactive>(line_clear_delay_poll_result));
    }

    switch (m_entry_delay.poll()) {
        case EntryDelayPollResult::ShouldSpawn:
            spawn_next_tetromino();  // this is where we could possibly have lost the game
            m_lock_delay_state.clear();
            if (is_game_over()) {
                ++m_next_frame;
                return;
            }
            break;
        case EntryDelayPollResult::ShouldNotSpawn:
            break;
    }

    process_keys(key_state);

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
            freeze_and_destroy_active_tetromino();  // we could lose the game here due to "Lock Out"
            m_is_hold_possible = true;
            // Even if we lost the game, it's not an error to start the entry delay -- it will simply get
            // ignored at the start of the next frame.
            m_entry_delay.start();
            break;
        case LockDelayPollResult::ShouldNotLock:
            break;
    }

    switch (m_auto_shift_state.poll()) {
        using enum AutoShiftDirection;
        case Left:
            move_left();
            break;
        case Right:
            move_right();
            break;
        case None:
            break;
    }
    determine_lines_to_clear();

    refresh_ghost_tetromino();

    ++m_next_frame;
}

[[nodiscard]] std::vector<ObserverTetrion*> ObpfTetrion::get_observers() const {
    return {};
}

void ObpfTetrion::on_client_disconnected(u8) {}

[[nodiscard]] LineClearDelay::State ObpfTetrion::line_clear_delay_state() const {
    return m_line_clear_delay.state();
}

[[nodiscard]] std::array<TetrominoType, 6> ObpfTetrion::get_preview_tetrominos() const {
    auto result = std::array<TetrominoType, 6>{};

    auto index_within_bag = m_bag_index;
    auto index_of_bag = std::size_t{ 0 };
    for (auto i = std::size_t{ 0 }; i < result.size(); ++i) {
        result.at(i) = m_bags.at(index_of_bag).tetrominos.at(index_within_bag);
        ++index_within_bag;
        if (index_within_bag >= std::tuple_size_v<decltype(Bag::tetrominos)>) {
            index_within_bag = 0;
            ++index_of_bag;
            assert(index_of_bag < m_bags.size());
        }
    }

    return result;
}

[[nodiscard]] std::optional<TetrominoType> ObpfTetrion::hold_piece() const {
    return m_hold_piece;
}

[[nodiscard]] u32 ObpfTetrion::level() const {
    return m_num_lines_cleared / 10;
}

void ObpfTetrion::freeze_and_destroy_active_tetromino() {
    if (not active_tetromino().has_value()) {
        return;
    }
    auto const mino_positions = get_mino_positions(active_tetromino().value());
    if (is_tetromino_completely_invisible(active_tetromino().value())) {
        m_is_game_over = true;
    }
    for (auto const position : mino_positions) {
        m_matrix[position] = active_tetromino().value().type;
    }
    m_active_tetromino = std::nullopt;
}

bool ObpfTetrion::is_tetromino_completely_invisible(Tetromino const& tetromino) const {
    return std::ranges::all_of(get_mino_positions(tetromino), [](auto const position) {
        return position.y < gsl::narrow<i32>(Matrix::num_invisible_lines);
    });
}

[[nodiscard]] bool ObpfTetrion::is_tetromino_completely_visible(Tetromino const& tetromino) const {
    return is_tetromino_position_valid(tetromino)
           and std::ranges::all_of(get_mino_positions(tetromino), [](auto const position) {
                   return position.y >= gsl::narrow<i32>(Matrix::num_invisible_lines);
               });
}

[[nodiscard]] bool ObpfTetrion::is_tetromino_position_valid(Tetromino const& tetromino) const {
    // clang-format off
    return std::ranges::all_of(
        get_mino_positions(tetromino),
        [this](auto const position) {
            return position.x >= 0
                and position.x < gsl::narrow<i32>(Matrix::width)
                and position.y >= 0
                and position.y < gsl::narrow<i32>(Matrix::height)
                and m_matrix[position] == TetrominoType::Empty;
        }
    );
    // clang-format on
}

[[nodiscard]] bool ObpfTetrion::is_active_tetromino_position_valid() const {
    return not active_tetromino().has_value() or is_tetromino_position_valid(active_tetromino().value());
}

void ObpfTetrion::spawn_next_tetromino() {
    if (m_old_hold_piece.has_value()) {
        m_active_tetromino = Tetromino{ spawn_position, spawn_rotation, m_old_hold_piece.value() };
        m_old_hold_piece.reset();
    } else {
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

    if (not is_active_tetromino_position_valid()) {
        m_is_game_over = true;
        m_is_soft_dropping = false;
        return;
    }

    // clang-format off
    for (
        auto i = std::size_t{ 0 };
        not is_tetromino_completely_visible(active_tetromino().value()) and i < Matrix::num_invisible_lines;
        ++i
    ) {  // clang-format on
        m_active_tetromino->position.y += 1;
        if (not is_active_tetromino_position_valid()) {
            m_active_tetromino->position.y -= 1;
            break;
        }
    }

    m_is_soft_dropping = false;
    m_next_gravity_frame = m_next_frame + gravity_delay_by_level(level());
}

void ObpfTetrion::process_keys(KeyState const key_state) {
    using std::ranges::views::enumerate;
    using std::ranges::views::filter;

    auto const pressed_keys = determine_pressed_keys(m_last_key_state, key_state);
    auto const released_keys = determine_released_keys(m_last_key_state, key_state);
    m_last_key_state = key_state;

    /* To avoid certain kinds of errors, we have to process the different keys in a certain order. That is:
     * 1. hold
     * 2. sideways movement (left, right)
     *    - if hold was pressed, ignore the following keys:
     * 3. rotation
     * 4. soft drop
     * 5. hard drop */
    auto const is_key_pressed = [&pressed_keys](Key const key) {
        return pressed_keys.at(gsl::narrow<std::size_t>(std::to_underlying(key)));
    };
    auto const hold_pressed = is_key_pressed(Key::Hold);
    if (hold_pressed) {
        handle_key_press(Key::Hold);
    }
    for (auto const key : std::array{ Key::Left, Key::Right }) {
        if (is_key_pressed(key)) {
            handle_key_press(key);
        }
    }
    if (not hold_pressed) {
        for (auto const key : std::array{
                 Key::RotateClockwise,
                 Key::RotateCounterClockwise,
                 Key::Down,
                 Key::Drop,
             }) {
            if (is_key_pressed(key)) {
                handle_key_press(key);
            }
        }
    }

    for (auto const [i, is_released] : filter(enumerate(released_keys), [](auto const tuple) { return get<1>(tuple); })) {
        auto const key = magic_enum::enum_cast<Key>(gsl::narrow<std::underlying_type_t<Key>>(i));
        assert(key.has_value());
        handle_key_release(key.value());
    }
}

void ObpfTetrion::handle_key_press(Key const key) {
    switch (key) {
        using enum Key;
        case Left:
            m_auto_shift_state.left_pressed();
            break;
        case Right:
            m_auto_shift_state.right_pressed();
            break;
        case Down:
            m_is_soft_dropping = true;
            m_next_gravity_frame = m_next_frame;
            break;
        case Drop:
            hard_drop();
            break;
        case RotateClockwise:
            rotate_clockwise();
            break;
        case RotateCounterClockwise:
            rotate_counter_clockwise();
            break;
        case Hold:
            hold();
            break;
    }
}

void ObpfTetrion::handle_key_release(Key const key) {
    switch (key) {
        using enum Key;
        case Left:
            m_auto_shift_state.left_released();
            return;
        case Right:
            m_auto_shift_state.right_released();
            return;
        case Down:
            m_is_soft_dropping = false;
            m_next_gravity_frame = m_next_frame + gravity_delay_by_level(level());
            return;
        case Drop:
        case RotateClockwise:
        case RotateCounterClockwise:
        case Hold:
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
        if (m_lock_delay_state.on_tetromino_moved(LockDelayMovementType::NotMovedDown)
            == LockDelayEventResult::HasTouched) {
            on_touch_event();
        }
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
        if (m_lock_delay_state.on_tetromino_moved(LockDelayMovementType::NotMovedDown)
            == LockDelayEventResult::HasTouched) {
            on_touch_event();
        }
    } else {
        --m_active_tetromino.value().position.x;
    }
}

void ObpfTetrion::move_down(DownMovementType const movement_type) {
    using enum DownMovementType;
    using enum LockDelayEventResult;

    if (not active_tetromino().has_value()) {
        return;
    }
    ++m_active_tetromino.value().position.y;
    if (is_active_tetromino_position_valid()) {
        if (m_lock_delay_state.on_tetromino_moved(LockDelayMovementType::MovedDown) == HasTouched) {
            on_touch_event();
        }
        if (movement_type == SoftDrop) {
            ++m_score;
        }
    } else {
        --m_active_tetromino.value().position.y;
        switch (movement_type) {
            case Gravity:
                if (m_lock_delay_state.on_gravity_lock() == HasTouched) {
                    on_touch_event();
                }
                break;
            case SoftDrop:
                if (m_lock_delay_state.on_soft_drop_lock() == HasTouched) {
                    on_touch_event();
                }
                m_is_soft_dropping = false;
                break;
        }
    }
}

void ObpfTetrion::rotate(RotationDirection const direction) {
    using enum LockDelayMovementType;
    using enum LockDelayEventResult;

    if (not m_active_tetromino.has_value()) {
        return;
    }
    auto const from_rotation = m_active_tetromino->rotation;
    auto const to_rotation = from_rotation + direction;
    m_active_tetromino->rotation = to_rotation;

    for (auto const translation : get_wall_kick_table(m_active_tetromino->type, from_rotation, to_rotation)) {
        m_active_tetromino->position += translation;
        if (is_active_tetromino_position_valid()) {
            if (m_lock_delay_state.on_tetromino_moved(NotMovedDown) == HasTouched) {
                on_touch_event();
            }
            if (m_action_handler != nullptr) {
                m_action_handler(
                    static_cast<ObpfAction>(
                        direction == RotationDirection::Clockwise ? Action::RotateCW : Action::RotateCCW
                    ),
                    m_action_handler_user_data
                );
            }
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

void ObpfTetrion::hard_drop() {
    if (not active_tetromino().has_value()) {
        return;
    }
    auto num_lines_dropped = std::size_t{ 0 };
    do {
        ++m_active_tetromino.value().position.y;
        ++num_lines_dropped;
    } while (is_active_tetromino_position_valid());
    --m_active_tetromino.value().position.y;
    --num_lines_dropped;
    static constexpr auto score_per_line = u64{ 2 };
    m_score += num_lines_dropped * score_per_line;
    if (m_lock_delay_state.on_hard_drop_lock() == LockDelayEventResult::HasTouched) {
        on_touch_event();
    }

    if (m_action_handler != nullptr) {
        m_action_handler(static_cast<ObpfAction>(Action::HardDrop), m_action_handler_user_data);
    }
}

void ObpfTetrion::hold() {
    if (not m_is_hold_possible or not m_active_tetromino.has_value()) {
        return;
    }
    if (m_hold_piece.has_value()) {
        m_entry_delay.spawn_next_frame();
    } else {
        m_entry_delay.start();
    }
    m_old_hold_piece = std::exchange(m_hold_piece, m_active_tetromino.value().type);
    m_active_tetromino = std::nullopt;
    m_is_hold_possible = false;
}

void ObpfTetrion::determine_lines_to_clear() {
    auto lines_to_clear = c2k::StaticVector<u8, 4>{};
    for (auto i = std::size_t{ 0 }; i < Matrix::height; ++i) {
        auto const line = Matrix::height - i - 1;
        if (not m_matrix.is_line_full(line)) {
            continue;
        }

        lines_to_clear.push_back(gsl::narrow<u8>(line));
    }

    if (not lines_to_clear.empty()) {
        m_line_clear_delay.start(lines_to_clear);
        if (m_action_handler) {
            m_action_handler(
                static_cast<ObpfAction>(std::to_underlying(Action::Clear1) + lines_to_clear.size() - 1),
                m_action_handler_user_data
            );
        }
    }
}

[[nodiscard]] u64 ObpfTetrion::score_for_num_lines_cleared(std::size_t const num_lines_cleared) const {
    static constexpr auto score_multipliers = std::array<std::size_t, 5>{ 0, 100, 300, 500, 800 };
    return score_multipliers.at(num_lines_cleared) * (level() + 1);
}

void ObpfTetrion::clear_lines(c2k::StaticVector<u8, 4> const lines) {
    assert(not lines.empty());
    m_score += score_for_num_lines_cleared(lines.size());
    auto num_lines_cleared = decltype(lines.front()){ 0 };
    for (auto const line_to_clear : lines) {
        for (auto i = decltype(line_to_clear){ 0 }; i < line_to_clear; ++i) {
            auto const line = gsl::narrow<usize>(line_to_clear - i + num_lines_cleared);
            m_matrix.copy_line(line, line - 1);
        }
        ++num_lines_cleared;
        m_matrix.fill(num_lines_cleared, TetrominoType::Empty);
    }
    m_num_lines_cleared += gsl::narrow<decltype(m_num_lines_cleared)>(lines.size());
    if (m_matrix.is_empty() and m_action_handler != nullptr) {
        m_action_handler(static_cast<ObpfAction>(Action::AllClear), m_action_handler_user_data);
    }
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

void ObpfTetrion::on_touch_event() const {
    if (m_action_handler != nullptr) {
        m_action_handler(static_cast<ObpfAction>(Action::Touch), m_action_handler_user_data);
    }
}

[[nodiscard]] std::array<Bag, 2> ObpfTetrion::create_two_bags(std::mt19937_64& random) {
    auto const bag0 = Bag{ random };
    auto const bag1 = Bag{ random };
    return std::array{ bag0, bag1 };
}
