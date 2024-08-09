#include <gtest/gtest.h>
#include <simulator/tetrion.hpp>

static constexpr u64 seed_for_tetromino_type(TetrominoType const type) {
    switch (type) {
        case TetrominoType::I:
            return 13;
        case TetrominoType::J:
            return 10;
        case TetrominoType::L:
            return 11;
        case TetrominoType::O:
            return 1;
        case TetrominoType::S:
            return 0;
        case TetrominoType::T:
            return 22;
        case TetrominoType::Z:
            return 4;
        case TetrominoType::Empty:
            throw std::runtime_error{ "Empty type cannot be spawned" };
    }
    throw std::runtime_error{ "Invalid TetrominoType" };
}

static constexpr char to_char(TetrominoType const type) {
    switch (type) {
        case TetrominoType::Empty:
            return ' ';
        case TetrominoType::I:
            return 'I';
        case TetrominoType::J:
            return 'J';
        case TetrominoType::L:
            return 'L';
        case TetrominoType::O:
            return 'O';
        case TetrominoType::S:
            return 'S';
        case TetrominoType::T:
            return 'T';
        case TetrominoType::Z:
            return 'Z';
    }
    throw std::runtime_error{ "Invalid TetrominoType" };
}

[[maybe_unused]] static void render_tetrion(ObpfTetrion const& tetrion) {
    auto const active_minos =
        tetrion.active_tetromino().transform([](Tetromino const& tetromino) { return get_mino_positions(tetromino); });

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
next_column:  // 👈(ﾟヮﾟ👈)
            (void)1;
        }
        std::cout << '\n';
    }
}

TEST(TetrionTests, AllClear) {
    auto tetrion = ObpfTetrion{ seed_for_tetromino_type(TetrominoType::I) };
    auto called_count = usize{ 0 };
    tetrion.set_action_handler(
        [](Action const action, void* user_data) {
            if (action == Action::AllClear) {
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
        tetrion.simulate_next_frame(KeyState{});
    }
    tetrion.simulate_next_frame(KeyState{
        false,
        false,
        false,
        false,
        true,
        false,
        false,
    });

    while (tetrion.active_tetromino().has_value()) {
        tetrion.simulate_next_frame(KeyState{
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
        tetrion.simulate_next_frame(KeyState{});
    }

    EXPECT_EQ(called_count, 1);
    EXPECT_TRUE(tetrion.matrix().is_empty());
}
