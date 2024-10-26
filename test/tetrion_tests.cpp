#include <gtest/gtest.h>
#include <ranges>
#include <simulator/tetrion.hpp>

static constexpr u64 seed_for_tetromino_type(TetrominoType const type) {
    switch (type) {
        using enum TetrominoType;
        case I:
            return 13;
        case J:
            return 10;
        case L:
            return 11;
        case O:
            return 1;
        case S:
            return 0;
        case T:
            return 22;
        case Z:
            return 4;
        case Garbage:
        case Empty:
            throw std::runtime_error{ "Empty type cannot be spawned" };
    }
    throw std::runtime_error{ "Invalid TetrominoType" };
}

static constexpr char to_char(TetrominoType const type) {
    switch (type) {
        using enum TetrominoType;
        case Empty:
            return ' ';
        case I:
            return 'I';
        case J:
            return 'J';
        case L:
            return 'L';
        case O:
            return 'O';
        case S:
            return 'S';
        case T:
            return 'T';
        case Z:
            return 'Z';
        case Garbage:
            return 'G';
    }
    throw std::runtime_error{ "Invalid TetrominoType" };
}

[[maybe_unused]] static void render_tetrion(ObpfTetrion const& tetrion) {
    auto const active_minos =
        tetrion.active_tetromino().transform([](Tetromino const& tetromino) { return get_mino_positions(tetromino); });

    using std::ranges::views::cartesian_product;
    using std::ranges::views::iota;

    for (auto row = usize{ 0 }; row < Matrix::height; ++row) {
        for (auto column = usize{ 0 }; column < Matrix::width; ++column) {
            if (active_minos.has_value()) {
                for (auto const mino : active_minos.value()) {
                    if (mino == Vec2{ static_cast<i32>(column), static_cast<i32>(row) }) {
                        std::cout << to_char(tetrion.active_tetromino()->type);
                        goto next_column;  // (👉ﾟヮﾟ)👉
                    }
                }
            }
            std::cout << to_char(tetrion.matrix()[Vec2{ static_cast<i32>(column), static_cast<i32>(row) }]);
next_column:;  // 👈(ﾟヮﾟ👈)
        }
        std::cout << '\n';
    }
}

TEST(TetrionTests, AllClear) {
    auto tetrion = ObpfTetrion{ seed_for_tetromino_type(TetrominoType::I), 0 };
    auto called_count = usize{ 0 };
    tetrion.set_action_handler(
        [](ObpfAction const action, void* user_data) {
            if (action == static_cast<ObpfAction>(Action::AllClear)) {
                *static_cast<usize*>(user_data) = true;
            }
        },
        static_cast<void*>(&called_count)
    );
    for (auto row = Matrix::height - 4; row < Matrix::height; ++row) {
        for (auto column = usize{ 0 }; column < Matrix::width - 1; ++column) {
            tetrion.matrix()[Vec2{ static_cast<i32>(column), static_cast<i32>(row) }] = TetrominoType::I;
        }
    }

    while (not tetrion.active_tetromino().has_value()) {
        std::ignore = tetrion.simulate_next_frame(KeyState{});
    }
    std::ignore = tetrion.simulate_next_frame(KeyState{
        false,
        false,
        false,
        false,
        true,
        false,
        false,
    });

    while (tetrion.active_tetromino().has_value()) {
        std::ignore = tetrion.simulate_next_frame(KeyState{
            false,
            true,
            false,
            false,
            false,
            false,
            false,
        });
    }

    while (not tetrion.active_tetromino().has_value()) {
        std::ignore = tetrion.simulate_next_frame(KeyState{});
    }

    EXPECT_EQ(called_count, 1);
    EXPECT_TRUE(tetrion.matrix().is_empty());
}
