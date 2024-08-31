#pragma once

#include <array>
#include <lib2k/random.hpp>
#include "tetromino_type.hpp"

struct Bag final {
    std::array<TetrominoType, 7> tetrominos{};

    explicit Bag(std::mt19937_64& random) {
        using enum TetrominoType;
        tetrominos = std::array{
            I, J, L, O, S, T, Z,
        };

        // We cannot use std::shuffle here because it presumably uses std::uniform_int_distribution which is not
        // guaranteed to be deterministic across different standard libraries.
        shuffle(tetrominos, random);
    }

private:
    static void shuffle(decltype(tetrominos)& tetrominos, std::mt19937_64& engine) {
        for (auto i = tetrominos.end() - tetrominos.begin() - 1; i > 0; --i) {
            using std::swap;
            // Using operator% won't result in a uniform distribution, but that's fine for our purposes.
            swap(tetrominos.begin()[i], tetrominos.begin()[engine() % (static_cast<std::uint64_t>(i) + 1)]);
        }
    }
};
