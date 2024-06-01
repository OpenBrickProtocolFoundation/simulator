#pragma once

#include "tetromino_type.hpp"
#include "vec2.hpp"
#include <array>

class Matrix final {
public:
    static constexpr auto width = std::size_t{ 10 };
    static constexpr auto height = std::size_t{ 22 };

private:
    std::array<TetrominoType, width * height> m_minos{};

public:
    [[nodiscard]] TetrominoType operator[](Vec2 const index) const {
        return m_minos.at(index.y * width + index.x);
    }

    [[nodiscard]] TetrominoType& operator[](Vec2 const index) {
        return m_minos.at(index.y * width + index.x);
    }
};
