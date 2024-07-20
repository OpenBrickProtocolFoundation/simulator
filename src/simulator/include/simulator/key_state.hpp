#pragma once

#include <gsl/gsl>
#include <lib2k/types.hpp>
#include <magic_enum.hpp>
#include <optional>
#include <simulator/input.hpp>
#include <type_traits>
#include <utility>

class KeyState final {
private:
    u8 m_bitmask = 0;

    explicit constexpr KeyState(u8 const bitmask) noexcept
        : m_bitmask(bitmask) {}

public:
    constexpr KeyState() = default;

    constexpr KeyState(
        bool const left,
        bool const right,
        bool const down,
        bool const drop,
        bool const rotate_clockwise,
        bool const rotate_counter_clockwise,
        bool const hold
    ) noexcept
        : m_bitmask{ gsl::narrow<decltype(m_bitmask)>(
              (left << std::to_underlying(Key::Left)) | (right << std::to_underlying(Key::Right))
              | (down << std::to_underlying(Key::Down)) | (drop << std::to_underlying(Key::Drop))
              | (rotate_clockwise << std::to_underlying(Key::RotateClockwise))
              | (rotate_counter_clockwise << std::to_underlying(Key::RotateCounterClockwise))
              | (hold << std::to_underlying(Key::Hold))
          ) } {}

    [[nodiscard]] constexpr bool get(Key const key) const noexcept {
        return (m_bitmask & (1 << std::to_underlying(key))) != 0;
    }

    constexpr KeyState& set(Key const key, bool const value = true) & noexcept {
        if (value) {
            m_bitmask |= (1 << std::to_underlying(key));
        } else {
            m_bitmask &= ~(1 << std::to_underlying(key));
        }
        return *this;
    }

    constexpr KeyState set(Key const key, bool const value = true) && noexcept {
        if (value) {
            m_bitmask |= (1 << std::to_underlying(key));
        } else {
            m_bitmask &= ~(1 << std::to_underlying(key));
        }
        return *this;
    }

    [[nodiscard]] constexpr auto get_bitmask() const noexcept {
        return m_bitmask;
    }

    [[nodiscard]] static constexpr std::optional<KeyState> from_bitmask(u8 const bitmask) noexcept {
        for (auto offset = usize{ 0 }; offset < sizeof(bitmask); ++offset) {
            auto const bit = bitmask & (1 << offset);
            auto const is_set = (bit != 0);
            if (is_set) {
                if (not magic_enum::enum_contains<Key>(gsl::narrow<std::underlying_type_t<Key>>(offset))) {
                    return std::nullopt;
                }
            }
        }
        return KeyState{ bitmask };
    }

    [[nodiscard]] constexpr bool operator==(KeyState const other) const noexcept {
        return m_bitmask == other.m_bitmask;
    }
};
