#pragma once

#include <compare>
#include <cstdint>

struct Vec2 final {
    std::int32_t x;
    std::int32_t y;

    constexpr Vec2()
        : Vec2{ 0, 0 } {}

    constexpr Vec2(std::int32_t const x_, std::int32_t const y_)
        : x{ x_ }, y{ y_ } {}

    [[nodiscard]] friend constexpr Vec2 operator+(Vec2 const self, Vec2 const other) {
        return Vec2{ self.x + other.x, self.y + other.y };
    }

    [[nodiscard]] friend constexpr Vec2 operator-(Vec2 const self, Vec2 const other) {
        return Vec2{ self.x - other.x, self.y - other.y };
    }

    constexpr Vec2& operator+=(Vec2 const other) {
        *this = *this + other;
        return *this;
    }

    constexpr Vec2& operator-=(Vec2 const other) {
        *this = *this - other;
        return *this;
    }

    [[nodiscard]] friend auto operator<=>(Vec2 const&, Vec2 const&) = default;
};
