#pragma once

#include <cassert>
#include <functional>
#include <lib2k/types.hpp>

enum class LockDelayPollResult {
    ShouldLock,
    ShouldNotLock,
};

enum class LockDelayMovementType {
    MovedDown,
    NotMovedDown,
};

class LockDelayState final {
private:
    bool m_delay_active = false;
    u64 m_delay_counter = 0;
    u32 m_num_lock_delays_executed = 0;
    bool m_can_lock = false;
    std::function<void()> m_on_touch_handler;

    static constexpr auto delay = u64{ 30 };
    static constexpr auto max_num_lock_delays = u32{ 30 };

public:
    void set_on_touch_handler(std::function<void()> on_touch_handler) {
        m_on_touch_handler = std::move(on_touch_handler);
    }

    /**
     * Call this function when you would normally lock the tetromino because of applied gravity.
     */
    void on_gravity_lock() {
        m_can_lock = true;
        if (not m_delay_active) {
            m_delay_active = true;
            m_delay_counter = delay;
            m_num_lock_delays_executed = 1;
            if (m_on_touch_handler) {
                m_on_touch_handler();
            }
        }
    }

    /**
     * Call this function when you would normally lock the tetromino because of a soft drop
     * (but don't actually lock it).
     */
    void on_soft_drop_lock() {
        m_can_lock = true;
        if (not m_delay_active) {
            m_delay_active = true;
            m_delay_counter = delay;
            m_num_lock_delays_executed = 1;
            if (m_on_touch_handler) {
                m_on_touch_handler();
            }
        } else {
            on_hard_drop_lock();
        }
    }

    /**
     * Call this function when you would normally force lock the tetromino because of a hard drop
     * (but don't actually force lock it).
     */
    void on_hard_drop_lock() {
        m_can_lock = true;
        m_delay_active = true;
        m_delay_counter = 1;  // this forces the tetromino to lock immediately
        m_num_lock_delays_executed = max_num_lock_delays;  // should not be necessary, but just in case
        if (m_on_touch_handler) {
            m_on_touch_handler();
        }
    }

    /**
     * Each time a tetromino has successfully been moved, call this function.
     */
    void on_tetromino_moved(LockDelayMovementType const movement_type) {
        switch (movement_type) {
            case LockDelayMovementType::MovedDown:
                break;
            case LockDelayMovementType::NotMovedDown:
                if ((not m_delay_active) or (m_num_lock_delays_executed >= max_num_lock_delays)) {
                    return;
                }
                m_delay_counter = delay;
                ++m_num_lock_delays_executed;
                if (m_on_touch_handler) {
                    m_on_touch_handler();
                }
                break;
        }
    }

    /**
     * Must be called every simulation step. Returns whether the tetromino should be locked. The piece may only
     * ever be locked if this returns LockDelayPollResult::ShouldLock.
     * @return Whether the tetromino should be locked.
     */
    [[nodiscard]] LockDelayPollResult poll() {
        struct Cleanup {
            bool& can_lock;

            ~Cleanup() {
                can_lock = false;
            }
        } cleanup{ m_can_lock };

        if (not m_delay_active) {
            return LockDelayPollResult::ShouldNotLock;
        }

        if (m_delay_counter > 1) {
            --m_delay_counter;
            return LockDelayPollResult::ShouldNotLock;
        }

        assert(m_delay_counter == 1);
        if (m_can_lock) {
            m_delay_active = false;
            return LockDelayPollResult::ShouldLock;
        }
        return LockDelayPollResult::ShouldNotLock;
    }

    void clear() {
        auto handler = std::move(m_on_touch_handler);
        *this = {};
        m_on_touch_handler = std::move(handler);
    }
};
