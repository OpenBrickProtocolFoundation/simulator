#include <cassert>
#include <lib2k/types.hpp>
#include <simulator/rotation.hpp>
#include <simulator/wallkicks.hpp>
#include <stdexcept>

using WallKickTable = std::array<std::array<Vec2, 5>, 8>;

static usize rotation_to_index(Rotation const from, Rotation const to) {
    if (from == Rotation::North and to == Rotation::East) {
        return 0;
    }
    if (from == Rotation::East and to == Rotation::North) {
        return 1;
    }
    if (from == Rotation::East and to == Rotation::South) {
        return 2;
    }
    if (from == Rotation::South and to == Rotation::East) {
        return 3;
    }
    if (from == Rotation::South and to == Rotation::West) {
        return 4;
    }
    if (from == Rotation::West and to == Rotation::South) {
        return 5;
    }
    if (from == Rotation::West and to == Rotation::North) {
        return 6;
    }
    if (from == Rotation::North and to == Rotation::West) {
        return 7;
    }
    std::unreachable();
}

static constexpr auto wall_kick_data_jltsz = WallKickTable{
    // North -> East
    std::array{
               Vec2{ 0, 0 },
               Vec2{ -1, 0 },
               Vec2{ -1, -1 },
               Vec2{ 0, 2 },
               Vec2{ -1, 2 },
               },
    // East -> North
    std::array{
               Vec2{ 0, 0 },
               Vec2{ 1, 0 },
               Vec2{ 1, 1 },
               Vec2{ 0, -2 },
               Vec2{ 1, -2 },
               },
    // East -> South
    std::array{
               Vec2{ 0, 0 },
               Vec2{ 1, 0 },
               Vec2{ 1, 1 },
               Vec2{ 0, -2 },
               Vec2{ 1, -2 },
               },
    // South -> East
    std::array{
               Vec2{ 0, 0 },
               Vec2{ -1, 0 },
               Vec2{ -1, -1 },
               Vec2{ 0, 2 },
               Vec2{ -1, 2 },
               },
    // South -> West
    std::array{
               Vec2{ 0, 0 },
               Vec2{ 1, 0 },
               Vec2{ 1, -1 },
               Vec2{ 0, 2 },
               Vec2{ 1, 2 },
               },
    // West -> South
    std::array{
               Vec2{ 0, 0 },
               Vec2{ -1, 0 },
               Vec2{ -1, 1 },
               Vec2{ 0, -2 },
               Vec2{ -1, -2 },
               },
    // West -> North
    std::array{
               Vec2{ 0, 0 },
               Vec2{ -1, 0 },
               Vec2{ -1, 1 },
               Vec2{ 0, -2 },
               Vec2{ -1, -2 },
               },
    // North -> West
    std::array{
               Vec2{ 0, 0 },
               Vec2{ 1, 0 },
               Vec2{ 1, -1 },
               Vec2{ 0, 2 },
               Vec2{ 1, 2 },
               },
};

static constexpr auto wall_kick_data_i = WallKickTable{
    // North -> East
    std::array{
               Vec2{ 0, 0 },
               Vec2{ -2, 0 },
               Vec2{ 1, 0 },
               Vec2{ -2, 1 },
               Vec2{ 1, -2 },
               },
    // East -> North
    std::array{
               Vec2{ 0, 0 },
               Vec2{ 2, 0 },
               Vec2{ -1, 0 },
               Vec2{ 2, -1 },
               Vec2{ -1, 2 },
               },
    // East -> South
    std::array{
               Vec2{ 0, 0 },
               Vec2{ -1, 0 },
               Vec2{ 2, 0 },
               Vec2{ -1, -2 },
               Vec2{ 2, 1 },
               },
    // South -> East
    std::array{
               Vec2{ 0, 0 },
               Vec2{ 1, 0 },
               Vec2{ -2, 0 },
               Vec2{ 1, 2 },
               Vec2{ -2, -1 },
               },
    // South -> West
    std::array{
               Vec2{ 0, 0 },
               Vec2{ 2, 0 },
               Vec2{ -1, 0 },
               Vec2{ 2, -1 },
               Vec2{ -1, 2 },
               },
    // West -> South
    std::array{
               Vec2{ 0, 0 },
               Vec2{ -2, 0 },
               Vec2{ 1, 0 },
               Vec2{ -2, 1 },
               Vec2{ 1, -2 },
               },
    // West -> North
    std::array{
               Vec2{ 0, 0 },
               Vec2{ 1, 0 },
               Vec2{ -2, 0 },
               Vec2{ 1, 2 },
               Vec2{ -2, -1 },
               },
    // North -> West
    std::array{
               Vec2{ 0, 0 },
               Vec2{ -1, 0 },
               Vec2{ 2, 0 },
               Vec2{ -1, -2 },
               Vec2{ 2, 1 },
               },
};

[[nodiscard]] std::array<Vec2, 5> const& get_wall_kick_table(
    TetrominoType const tetromino_type,
    Rotation const from_rotation,
    Rotation const to_rotation
) {
    auto const index = rotation_to_index(from_rotation, to_rotation);
    switch (tetromino_type) {
        case TetrominoType::J:
        case TetrominoType::L:
        case TetrominoType::T:
        case TetrominoType::S:
        case TetrominoType::Z:
        case TetrominoType::O:  // O piece doesn't need the table, but this way the code is simpler
            return wall_kick_data_jltsz.at(index);
        case TetrominoType::I:
            return wall_kick_data_i.at(index);
        case TetrominoType::Empty:
            throw std::runtime_error{ "tetromino type must not be empty" };
    }
    std::unreachable();
}
