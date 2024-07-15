#pragma once

#include <lib2k/types.hpp>
#include <variant>

// https://tetris.wiki/Line_clear#Delay
class LineClearDelay final {
public:
    static constexpr auto delay = u64{ 24 };

    struct DelayIsActive {};

    struct DelayIsInactive {};

    struct DelayEnded {
        c2k::StaticVector<u8, 4> lines;
    };

    using LineClearDelayPollResult = std::variant<DelayIsActive, DelayIsInactive, DelayEnded>;

private:
    u64 m_countdown = 0;
    std::optional<c2k::StaticVector<u8, 4>> m_lines_to_clear;

public:
    [[nodiscard]] LineClearDelayPollResult poll() {
        if (m_countdown == 1) {
            assert(m_lines_to_clear.has_value());
            auto const lines = m_lines_to_clear.value();
            m_lines_to_clear.reset();
            m_countdown = 0;
            return DelayEnded{ lines };
        }
        if (m_countdown > 0) {
            --m_countdown;
            return DelayIsActive{};
        }
        return DelayIsInactive{};
    }

    void start(c2k::StaticVector<u8, 4> const lines_to_clear) {
        m_lines_to_clear = lines_to_clear;
        m_countdown = delay;
    }
};
