#pragma once

#include <cassert>
#include <cstdint>

enum class AutoShiftDirection {
    Left,
    Right,
    None,
};

class DelayedAutoShiftState final {
private:
    bool m_left_is_held_down = false;
    bool m_right_is_held_down = false;
    AutoShiftDirection m_direction = AutoShiftDirection::None;
    std::uint64_t m_counter = 0;
    std::uint64_t m_frame_delay_index = 0;

    static constexpr auto frame_delays = std::array{
        std::uint64_t{ 1 },
        std::uint64_t{ 10 },
        std::uint64_t{ 2 },
    };

public:
    [[nodiscard]] AutoShiftDirection poll() {
        if (m_counter == 0) {
            return AutoShiftDirection::None;
        }
        if (m_counter > 1) {
            --m_counter;
            return AutoShiftDirection::None;
        }
        assert(m_direction != AutoShiftDirection::None);
        m_frame_delay_index = std::min(m_frame_delay_index + 1, frame_delays.size() - 1);
        m_counter = frame_delays.at(m_frame_delay_index);
        return m_direction;
    }

    void left_pressed() {
        m_left_is_held_down = true;
        if (m_right_is_held_down) {
            stop_movement();
        } else {
            start_movement(AutoShiftDirection::Left);
        }
    }

    void right_pressed() {
        m_right_is_held_down = true;
        if (m_left_is_held_down) {
            stop_movement();
        } else {
            start_movement(AutoShiftDirection::Right);
        }
    }

    void left_released() {
        m_left_is_held_down = false;
        if (m_right_is_held_down) {
            start_movement(AutoShiftDirection::Right);
        } else {
            stop_movement();
        }
    }

    void right_released() {
        m_right_is_held_down = false;
        if (m_left_is_held_down) {
            start_movement(AutoShiftDirection::Left);
        } else {
            stop_movement();
        }
    }

private:
    void start_movement(AutoShiftDirection const direction) {
        m_frame_delay_index = 0;
        m_counter = frame_delays.at(m_frame_delay_index);
        m_direction = direction;
    }

    void stop_movement() {
        m_counter = 0;
    }
};
