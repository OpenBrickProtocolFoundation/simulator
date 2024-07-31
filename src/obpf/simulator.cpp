#include <obpf/simulator.h>
#include <spdlog/spdlog.h>
#include <gsl/gsl>
#include <memory>
#include <simulator/matrix.hpp>
#include <simulator/tetrion.hpp>
#include <simulator/tetromino.hpp>

enum class TetrominoSelection {
    ActiveTetromino,
    GhostTetromino,
};

[[nodiscard]] static bool try_get_tetromino(
    ObpfTetrion const* const tetrion,
    ObpfTetromino* const out_tetromino,
    TetrominoSelection const selection
) {
    auto const tetromino =
        (selection == TetrominoSelection::ActiveTetromino ? tetrion->active_tetromino() : tetrion->ghost_tetromino());

    if (not tetromino.has_value()) {
        return false;
    }
    auto const& mino_positions = get_mino_positions(tetromino.value());
    auto const result = ObpfTetromino{
        .mino_positions = {
            ObpfVec2{ gsl::narrow<std::uint8_t>(mino_positions.at(0).x), gsl::narrow<std::uint8_t>(mino_positions.at(0).y), },
            ObpfVec2{ gsl::narrow<std::uint8_t>(mino_positions.at(1).x), gsl::narrow<std::uint8_t>(mino_positions.at(1).y), },
            ObpfVec2{ gsl::narrow<std::uint8_t>(mino_positions.at(2).x), gsl::narrow<std::uint8_t>(mino_positions.at(2).y), },
            ObpfVec2{ gsl::narrow<std::uint8_t>(mino_positions.at(3).x), gsl::narrow<std::uint8_t>(mino_positions.at(3).y), },
        },
        .type = static_cast<ObpfTetrominoType>(tetromino->type),
    };
    *out_tetromino = result;
    return true;
}

ObpfTetrion* obpf_create_tetrion(uint64_t const seed) {
    return new ObpfTetrion{ seed };
}

ObpfLineClearDelayState obpf_tetrion_get_line_clear_delay_state(ObpfTetrion const* tetrion) {
    auto [lines, countdown] = tetrion->line_clear_delay_state();
    auto const original_size = lines.size();
    while (lines.size() < decltype(lines)::capacity()) {
        lines.push_back(0);
    }
    return ObpfLineClearDelayState{
        .count = gsl::narrow<u8>(original_size),
        .lines{
               lines.at(0),
               lines.at(1),
               lines.at(2),
               lines.at(3),
               },
        .countdown = countdown,
        .delay = LineClearDelay::delay,
    };
}

bool obpf_tetrion_try_get_active_tetromino(ObpfTetrion const* const tetrion, ObpfTetromino* const out_tetromino) {
    return try_get_tetromino(tetrion, out_tetromino, TetrominoSelection::ActiveTetromino);
}

bool obpf_tetrion_try_get_ghost_tetromino(ObpfTetrion const* tetrion, ObpfTetromino* out_tetromino) {
    return try_get_tetromino(tetrion, out_tetromino, TetrominoSelection::GhostTetromino);
}

uint64_t obpf_tetrion_get_next_frame(ObpfTetrion const* const tetrion) {
    return tetrion->next_frame();
}

void obpf_tetrion_simulate_next_frame(ObpfTetrion* const tetrion, ObpfKeyState const key_state) {
    tetrion->simulate_next_frame(KeyState::from_bitmask(key_state.bitmask).value());
}

void obpf_destroy_tetrion(ObpfTetrion const* const tetrion) {
    delete tetrion;
}

uint8_t obpf_tetrion_width() {
    return uint8_t{ Matrix::width };
}

uint8_t obpf_tetrion_height() {
    return uint8_t{ Matrix::height };
}

uint8_t obpf_tetrion_num_invisible_lines() {
    return uint8_t{ Matrix::num_invisible_lines };
}

ObpfTetrominoType obpf_tetrion_matrix_get(ObpfTetrion const* const tetrion, ObpfVec2 const position) {
    auto const pos = Vec2{ position.x, position.y };
    return static_cast<ObpfTetrominoType>(tetrion->matrix()[pos]);
}

ObpfPreviewPieces obpf_tetrion_get_preview_pieces(ObpfTetrion const* tetrion) {
    auto const preview_tetrominos = tetrion->get_preview_tetrominos();
    ObpfPreviewPieces result{};
    for (usize i = 0; i < preview_tetrominos.size(); ++i) {
        result.types[i] = static_cast<ObpfTetrominoType>(preview_tetrominos.at(i));
    }
    return result;
}

ObpfMinoPositions obpf_tetromino_get_mino_positions(ObpfTetrominoType const type, ObpfRotation const rotation) {
    auto const tetromino = Tetromino{
        Vec2{ 0, 0 },
        static_cast<Rotation>(rotation),
        static_cast<TetrominoType>(type)
    };
    auto const mino_positions = get_mino_positions(tetromino);
    ObpfMinoPositions result{};
    for (usize i = 0; i < mino_positions.size(); ++i) {
        result.positions[i] = ObpfVec2{
            gsl::narrow<std::uint8_t>(mino_positions.at(i).x),
            gsl::narrow<std::uint8_t>(mino_positions.at(i).y),
        };
    }
    return result;
}

ObpfTetrominoType obpf_tetrion_get_hold_piece(ObpfTetrion const* const tetrion) {
    auto const hold_piece = tetrion->hold_piece();
    if (hold_piece.has_value()) {
        return static_cast<ObpfTetrominoType>(hold_piece.value());
    }
    return OBPF_TETROMINO_TYPE_EMPTY;
}

ObpfKeyState obpf_key_state_create(
    bool const left,
    bool const right,
    bool const down,
    bool const drop,
    bool const rotate_clockwise,
    bool const rotate_counter_clockwise,
    bool const hold
) {
    return ObpfKeyState{
        .bitmask = KeyState{ left, right, down, drop, rotate_clockwise, rotate_counter_clockwise, hold }
          .get_bitmask(),
    };
}

ObpfStats obpf_tetrion_get_stats(ObpfTetrion const* tetrion) {
    return ObpfStats{
        .score = tetrion->score(),
        .lines_cleared = tetrion->num_lines_cleared(),
        .level = tetrion->level(),
    };
}

bool obpf_tetrion_is_game_over(ObpfTetrion const* const tetrion) {
    return tetrion->is_game_over();
}

void obpf_tetrion_set_action_handler(ObpfTetrion* const tetrion, ObpfActionHandler const handler, void* const user_data) {
    tetrion->set_action_handler(reinterpret_cast<ObpfTetrion::ActionHandler>(handler), user_data);
}
