#pragma once

#include <compare>
#include <cstdint>
#include <gsl/gsl>

struct Vec2 {
    std::uint8_t x;
    std::uint8_t y;

    constexpr Vec2() : Vec2{ 0, 0 } { }

    constexpr Vec2(std::uint8_t const x_, std::uint8_t const y_) : x{ x_ }, y{ y_ } { }

    [[nodiscard]] friend constexpr Vec2 operator+(Vec2 const self, Vec2 const other) {
        return Vec2{ gsl::narrow_cast<std::uint8_t>(self.x + other.x),
                     gsl::narrow_cast<std::uint8_t>(self.y + other.y) };
    }

    [[nodiscard]] friend constexpr Vec2 operator-(Vec2 const self, Vec2 const other) {
        return Vec2{ gsl::narrow_cast<std::uint8_t>(self.x - other.x),
                     gsl::narrow_cast<std::uint8_t>(self.y - other.y) };
    }

    [[nodiscard]] friend auto operator<=>(Vec2 const&, Vec2 const&) = default;
};
