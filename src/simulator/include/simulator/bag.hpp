#pragma once

#include <../../../../cmake-build-msvc-debug/_deps/lib2k-src/src/include/lib2k/random.hpp>
#include <array>
#include <iostream>
#include "tetromino_type.hpp"

struct Bag final {
    std::array<TetrominoType, 7> tetrominos{};

    explicit Bag(c2k::Random& random) {
        tetrominos = std::array{
            TetrominoType::I, TetrominoType::J, TetrominoType::L, TetrominoType::O,
            TetrominoType::S, TetrominoType::T, TetrominoType::Z,
        };

        random.shuffle(tetrominos);

        for (auto const& tetromino : tetrominos) {
            std::cerr << static_cast<int>(tetromino) << ' ';
        }
        std::cerr << '\n';
    }
};
