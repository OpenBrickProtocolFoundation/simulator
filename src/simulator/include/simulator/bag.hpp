#pragma once

#include "tetromino_type.hpp"
#include <../../../../cmake-build-msvc-debug/_deps/lib2k-src/src/include/lib2k/random.hpp>
#include <array>

struct Bag final {
    std::array<TetrominoType, 7> tetrominos{};

    explicit Bag(c2k::Random& random) {
        tetrominos = std::array{
            TetrominoType::I, TetrominoType::J, TetrominoType::L, TetrominoType::O,
            TetrominoType::S, TetrominoType::T, TetrominoType::Z,
        };

        random.shuffle(tetrominos);
    }
};
