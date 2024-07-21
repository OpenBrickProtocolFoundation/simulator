#pragma once

#include <lib2k/types.hpp>

enum class EntryDelayPollResult {
    ShouldSpawn,
    ShouldNotSpawn,
};

// https://tetris.wiki/Tetris_Guideline#Recommended_but_non-mandatory
class EntryDelay final {
private:
    static constexpr auto entry_delay = u64{ 6 };
    u64 m_countdown = 0;

public:
    [[nodiscard]] EntryDelayPollResult poll() {
        if (m_countdown > 1) {
            --m_countdown;
            return EntryDelayPollResult::ShouldNotSpawn;
        }
        if (m_countdown == 1) {
            m_countdown = 0;
            return EntryDelayPollResult::ShouldSpawn;
        }
        return EntryDelayPollResult::ShouldNotSpawn;
    }

    void start() {
        m_countdown = entry_delay;
    }

    void spawn_next_frame() {
        m_countdown = 1;
    }
};
