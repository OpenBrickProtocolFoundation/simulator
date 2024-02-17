#pragma once

#include "tetromino_type.hpp"
#include "vec2.hpp"
#include <array>
#include <obpf/constants.h>

struct ObpfMatrix final {
public:
    static constexpr auto width = OBPF_MATRIX_WIDTH;
    static constexpr auto height = OBPF_MATRIX_HEIGHT;

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
