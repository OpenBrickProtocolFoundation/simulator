#pragma once

#include <cassert>
#include <lib2k/defer.hpp>
#include <lib2k/types.hpp>

enum class LockDelayPollResult {
    ShouldLock,
    ShouldNotLock,
};

enum class LockDelayMovementType {
    MovedDown,
    NotMovedDown,
};

enum class LockDelayEventResult {
    HasTouched,
    HasNotTouched,
};

class LockDelayState final {
private:
    bool m_delay_active = false;
    u64 m_delay_counter = 0;
    u32 m_num_lock_delays_executed = 0;
    bool m_can_lock = false;

    static constexpr auto delay = u64{ 30 };
    static constexpr auto max_num_lock_delays = u32{ 30 };

public:
    /**
     * Call this function when you would normally lock the tetromino because of applied gravity.
     */
    [[nodiscard]] LockDelayEventResult on_gravity_lock() {
        m_can_lock = true;
        if (m_delay_active) {
            return LockDelayEventResult::HasNotTouched;
        }
        m_delay_active = true;
        m_delay_counter = delay;
        m_num_lock_delays_executed = 1;
        return LockDelayEventResult::HasTouched;
    }

    /**
     * Call this function when you would normally lock the tetromino because of a soft drop
     * (but don't actually lock it).
     */
    [[nodiscard]] LockDelayEventResult on_soft_drop_lock() {
        m_can_lock = true;
        if (m_delay_active) {
            return on_hard_drop_lock();
        }
        m_delay_active = true;
        m_delay_counter = delay;
        m_num_lock_delays_executed = 1;
        return LockDelayEventResult::HasTouched;
    }

    /**
     * Call this function when you would normally force lock the tetromino because of a hard drop
     * (but don't actually force lock it).
     */
    [[nodiscard]] LockDelayEventResult on_hard_drop_lock() {
        m_can_lock = true;
        m_delay_active = true;
        m_delay_counter = 1;  // this forces the tetromino to lock immediately
        m_num_lock_delays_executed = max_num_lock_delays;  // should not be necessary, but just in case
        return LockDelayEventResult::HasTouched;
    }

    /**
     * Each time a tetromino has successfully been moved, call this function.
     */
    [[nodiscard]] LockDelayEventResult on_tetromino_moved(LockDelayMovementType const movement_type) {
        switch (movement_type) {
            using enum LockDelayMovementType;
            using enum LockDelayEventResult;
            case MovedDown:
                return HasNotTouched;
            case NotMovedDown:
                if ((not m_delay_active) or (m_num_lock_delays_executed >= max_num_lock_delays)) {
                    return HasNotTouched;
                }
                m_delay_counter = delay;
                ++m_num_lock_delays_executed;
                return HasTouched;
        }
        throw std::runtime_error{ "unreachable" };
    }

    /**
     * Must be called every simulation step. Returns whether the tetromino should be locked. The piece may only
     * ever be locked if this returns LockDelayPollResult::ShouldLock.
     * @return Whether the tetromino should be locked.
     */
    [[nodiscard]] LockDelayPollResult poll() {
        using enum LockDelayPollResult;

        auto const _ = c2k::Defer{ [this] {
            m_can_lock = false;
        } };

        if (not m_delay_active) {
            return ShouldNotLock;
        }

        if (m_delay_counter > 1) {
            --m_delay_counter;
            return ShouldNotLock;
        }

        assert(m_delay_counter == 1);
        if (m_can_lock) {
            m_delay_active = false;
            return ShouldLock;
        }
        return ShouldNotLock;
    }

    void clear() {
        *this = {};
    }
};
