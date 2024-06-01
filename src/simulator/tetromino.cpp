#include "include/simulator/tetromino.hpp"

static constexpr std::size_t to_index(TetrominoType const type) {
    switch (type) {
        case TetrominoType::I:
            return 0;
        case TetrominoType::J:
            return 1;
        case TetrominoType::L:
            return 2;
        case TetrominoType::O:
            return 3;
        case TetrominoType::S:
            return 4;
        case TetrominoType::T:
            return 5;
        case TetrominoType::Z:
            return 6;
    }
    std::unreachable();
}

// clang-format off
static constexpr auto tetromino_patterns = std::array{
        // I
        std::array{
            std::array{ Vec2{ 0, 1 }, Vec2{ 1, 1 }, Vec2{ 2, 1 }, Vec2{ 3, 1 }, },
            std::array{ Vec2{ 2, 0 }, Vec2{ 2, 1 }, Vec2{ 2, 2 }, Vec2{ 2, 3 }, },
            std::array{ Vec2{ 0, 2 }, Vec2{ 1, 2 }, Vec2{ 2, 2 }, Vec2{ 3, 2 }, },
            std::array{ Vec2{ 1, 0 }, Vec2{ 1, 1 }, Vec2{ 1, 2 }, Vec2{ 1, 3 }, },
        },
        // J
        std::array{
            std::array{ Vec2{ 0, 0 }, Vec2{ 0, 1 }, Vec2{ 1, 1 }, Vec2{ 2, 1 }, },
            std::array{ Vec2{ 2, 0 }, Vec2{ 1, 0 }, Vec2{ 1, 1 }, Vec2{ 1, 2 }, },
            std::array{ Vec2{ 0, 1 }, Vec2{ 1, 1 }, Vec2{ 2, 1 }, Vec2{ 2, 2 }, },
            std::array{ Vec2{ 0, 2 }, Vec2{ 1, 2 }, Vec2{ 1, 1 }, Vec2{ 1, 0 }, },
        },
        // L
        std::array{
            std::array{ Vec2{ 0, 1 }, Vec2{ 1, 1 }, Vec2{ 2, 1 }, Vec2{ 2, 0 }, },
            std::array{ Vec2{ 1, 0 }, Vec2{ 1, 1 }, Vec2{ 1, 2 }, Vec2{ 2, 2 }, },
            std::array{ Vec2{ 0, 2 }, Vec2{ 0, 1 }, Vec2{ 1, 1 }, Vec2{ 2, 1 }, },
            std::array{ Vec2{ 0, 0 }, Vec2{ 1, 0 }, Vec2{ 1, 1 }, Vec2{ 1, 2 }, },
        },
        // O
        std::array{
            std::array{ Vec2{ 1, 0 }, Vec2{ 2, 0 }, Vec2{ 1, 1 }, Vec2{ 2, 1 }, },
            std::array{ Vec2{ 1, 0 }, Vec2{ 2, 0 }, Vec2{ 1, 1 }, Vec2{ 2, 1 }, },
            std::array{ Vec2{ 1, 0 }, Vec2{ 2, 0 }, Vec2{ 1, 1 }, Vec2{ 2, 1 }, },
            std::array{ Vec2{ 1, 0 }, Vec2{ 2, 0 }, Vec2{ 1, 1 }, Vec2{ 2, 1 }, },
        },
        // S
        std::array{
            std::array{ Vec2{ 0, 1 }, Vec2{ 1, 1 }, Vec2{ 1, 0 }, Vec2{ 2, 0 }, },
            std::array{ Vec2{ 1, 0 }, Vec2{ 1, 1 }, Vec2{ 2, 1 }, Vec2{ 2, 2 }, },
            std::array{ Vec2{ 0, 2 }, Vec2{ 1, 2 }, Vec2{ 1, 1 }, Vec2{ 2, 1 }, },
            std::array{ Vec2{ 0, 0 }, Vec2{ 0, 1 }, Vec2{ 1, 1 }, Vec2{ 1, 2 }, },
        },
        // T
        std::array{
            std::array{ Vec2{ 0, 1 }, Vec2{ 1, 1 }, Vec2{ 1, 0 }, Vec2{ 2, 1 }, },
            std::array{ Vec2{ 1, 0 }, Vec2{ 1, 1 }, Vec2{ 2, 1 }, Vec2{ 1, 2 }, },
            std::array{ Vec2{ 0, 1 }, Vec2{ 1, 1 }, Vec2{ 2, 1 }, Vec2{ 1, 2 }, },
            std::array{ Vec2{ 1, 0 }, Vec2{ 1, 1 }, Vec2{ 0, 1 }, Vec2{ 1, 2 }, },
        },
        // Z
        std::array{
            std::array{ Vec2{ 0, 0 }, Vec2{ 1, 0 }, Vec2{ 1, 1 }, Vec2{ 2, 1 }, },
            std::array{ Vec2{ 2, 0 }, Vec2{ 2, 1 }, Vec2{ 1, 1 }, Vec2{ 1, 2 }, },
            std::array{ Vec2{ 0, 1 }, Vec2{ 1, 1 }, Vec2{ 1, 2 }, Vec2{ 2, 2 }, },
            std::array{ Vec2{ 1, 0 }, Vec2{ 1, 1 }, Vec2{ 0, 1 }, Vec2{ 0, 2 }, },
        }
};
// clang-format on

[[nodiscard]] std::array<Vec2, 4> get_mino_positions(Tetromino const& tetromino) {
    auto result = tetromino_patterns.at(to_index(tetromino.type)).at(static_cast<std::size_t>(tetromino.rotation));
    for (auto& position : result) {
        position = position + tetromino.position;
    }
    return result;
}
