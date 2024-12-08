#include <obpf/simulator.h>
#include <spdlog/spdlog.h>
#include <gsl/gsl>
#include <memory>
#include <simulator/matrix.hpp>
#include <simulator/multiplayer_tetrion.hpp>
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

ObpfTetrion* obpf_create_tetrion(uint64_t const seed) try {
    return new ObpfTetrion{ seed, 0, Logging::Enabled };
} catch (std::exception const& e) {

    spdlog::error("Failed to create tetrion: {}", e.what());
    return nullptr;
} catch (...) {
    spdlog::error("Failed to create tetrion: Unknown error");
    return nullptr;
}

ObpfTetrion* obpf_create_multiplayer_tetrion(char const* const host, uint16_t const port, char const* const player_name) try {
    auto tetrion = MultiplayerTetrion::create(host, port, Logging::Enabled, player_name);
    if (tetrion == nullptr) {
        return nullptr;
    }
    return tetrion.release();
} catch (std::exception const& e) {

    spdlog::error("Failed to create multiplayer tetrion: {}", e.what());
    return nullptr;
} catch (...) {
    spdlog::error("Failed to create multiplayer tetrion: Unknown error");
    return nullptr;
}

ObpfObserverList obpf_tetrion_get_observers(struct ObpfTetrion const* tetrion) try {
    auto const observers = tetrion->get_observers();
    if (observers.empty()) {
        return ObpfObserverList{
            .num_observers = 0,
            .observers = nullptr,
        };
    }
    auto pointers = std::make_unique<ObpfTetrion*[]>(observers.size());
    std::copy_n(observers.begin(), observers.size(), pointers.get());
    return ObpfObserverList{
        .num_observers = observers.size(),
        .observers = pointers.release(),
    };
} catch (std::exception const& e) {

    spdlog::error("Failed to get observers: {}", e.what());
    return ObpfObserverList{
        .num_observers = 0,
        .observers = nullptr,
    };
} catch (...) {
    spdlog::error("Failed to get observers: Unknown error");
    return ObpfObserverList{
        .num_observers = 0,
        .observers = nullptr,
    };
}

void obpf_destroy_observers(ObpfObserverList const observers) try {
    [[maybe_unused]] auto const owner = std::unique_ptr<ObpfTetrion*[]>{ observers.observers };
} catch (std::exception const& e) {

    spdlog::error("Failed to destroy observers: {}", e.what());
} catch (...) {
    spdlog::error("Failed to destroy observers: Unknown error");
}

ObpfTetrion* obpf_clone_tetrion(ObpfTetrion const* const tetrion) try {
    auto result = new ObpfTetrion{ *tetrion };
    result->set_action_handler(nullptr, nullptr);
    return result;
} catch (std::exception const& e) {

    spdlog::error("Failed to clone tetrion: {}", e.what());
    return nullptr;
} catch (...) {
    spdlog::error("Failed to clone tetrion: Unknown error");
    return nullptr;
}

ObpfLineClearDelayState obpf_tetrion_get_line_clear_delay_state(ObpfTetrion const* tetrion) try {
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
} catch (std::exception const& e) {

    spdlog::error("Failed to get line clear delay state: {}", e.what());
    return ObpfLineClearDelayState{
        .count = 0,
        .lines{ 0, 0, 0, 0 },
        .countdown = 0,
        .delay = LineClearDelay::delay,
    };
} catch (...) {
    spdlog::error("Failed to get line clear delay state: Unknown error");
    return ObpfLineClearDelayState{
        .count = 0,
        .lines{ 0, 0, 0, 0 },
        .countdown = 0,
        .delay = LineClearDelay::delay,
    };
}

bool obpf_tetrion_try_get_active_tetromino(ObpfTetrion const* const tetrion, ObpfTetromino* const out_tetromino) try {
    return try_get_tetromino(tetrion, out_tetromino, TetrominoSelection::ActiveTetromino);
} catch (std::exception const& e) {

    spdlog::error("Failed to get active tetromino: {}", e.what());
    return false;
} catch (...) {
    spdlog::error("Failed to get active tetromino: Unknown error");
    return false;
}

bool obpf_tetrion_try_get_active_tetromino_transform(
    struct ObpfTetrion const* const tetrion,
    ObpfTetrominoType* const out_type,
    ObpfRotation* const out_rotation,
    ObpfVec2i* const out_position
) try {
    auto const tetromino = tetrion->active_tetromino();
    if (not tetromino.has_value()) {
        return false;
    }
    if (out_type != nullptr) {
        *out_type = static_cast<ObpfTetrominoType>(tetromino->type);
    }
    if (out_rotation != nullptr) {
        *out_rotation = static_cast<ObpfRotation>(tetromino->rotation);
    }
    if (out_position != nullptr) {
        *out_position = ObpfVec2i{
            tetromino->position.x,
            tetromino->position.y,
        };
    }
    return true;
} catch (std::exception const& e) {

    spdlog::error("Failed to get active tetromino transform: {}", e.what());
    return false;
} catch (...) {
    spdlog::error("Failed to get active tetromino transform: Unknown error");
    return false;
}

bool obpf_tetrion_try_get_ghost_tetromino(ObpfTetrion const* tetrion, ObpfTetromino* out_tetromino) try {
    return try_get_tetromino(tetrion, out_tetromino, TetrominoSelection::GhostTetromino);
} catch (std::exception const& e) {

    spdlog::error("Failed to get ghost tetromino: {}", e.what());
    return false;
} catch (...) {
    spdlog::error("Failed to get ghost tetromino: Unknown error");
    return false;
}

uint64_t obpf_tetrion_get_next_frame(ObpfTetrion const* const tetrion) try {
    return tetrion->next_frame();
} catch (std::exception const& e) {

    spdlog::error("Failed to get next frame: {}", e.what());
    return 0;
} catch (...) {
    spdlog::error("Failed to get next frame: Unknown error");
    return 0;
}

void obpf_tetrion_simulate_next_frame(ObpfTetrion* const tetrion, ObpfKeyState const key_state) try {
    // Ignoring the return value because the client application should not need to know anything about
    // the garbage that was sent. This should all be handled internally by the MultiplayerTetrion (in
    // case of being a client) or directly through the C++ API (in case of being a server).
    std::ignore = tetrion->simulate_next_frame(KeyState::from_bitmask(key_state.bitmask).value());
} catch (std::exception const& e) {

    spdlog::error("Failed to simulate next frame: {}", e.what());
} catch (...) {
    spdlog::error("Failed to simulate next frame: Unknown error");
}

void obpf_destroy_tetrion(ObpfTetrion const* const tetrion) try {
    if (tetrion == nullptr or tetrion->is_observer()) {
        return;
    }
    delete tetrion;
} catch (std::exception const& e) {

    spdlog::error("Failed to destroy tetrion: {}", e.what());
} catch (...) {
    spdlog::error("Failed to destroy tetrion: Unknown error");
}

std::uint32_t obpf_garbage_queue_length(ObpfTetrion const* const tetrion) {
    return tetrion->garbage_queue_length();
}

std::uint32_t obpf_garbage_queue_num_events(ObpfTetrion const* const tetrion) {
    return static_cast<std::uint32_t>(tetrion->garbage_queue_num_events());
}

ObpfGarbageEvent obpf_garbage_queue_event(ObpfTetrion const* const tetrion, uint32_t const index) try {
    auto const event = tetrion->garbage_queue_event(static_cast<std::size_t>(index));
    // we do the calculation with signed numbers to simplify the implementation
    auto const remaining_frames = static_cast<u64>(std::max(
        i64{ 0 },
        static_cast<i64>(event.frame + ObpfTetrion::garbage_delay_frames) - static_cast<i64>(tetrion->next_frame())
    ));
    return ObpfGarbageEvent{
        .num_lines = event.num_lines,
        .remaining_frames = remaining_frames,
    };
} catch (std::exception const& e) {

    spdlog::error("Failed to fetch garbage queue event: {}", e.what());
    return ObpfGarbageEvent{};
} catch (...) {
    spdlog::error("Failed to fetch garbage queue event: Unknown error");
    return ObpfGarbageEvent{};
}

std::uint8_t obpf_tetrion_width() {
    return uint8_t{ Matrix::width };
}

std::uint8_t obpf_tetrion_height() {
    return uint8_t{ Matrix::height };
}

std::uint8_t obpf_tetrion_num_invisible_lines() {
    return uint8_t{ Matrix::num_invisible_lines };
}

ObpfTetrominoType obpf_tetrion_matrix_get(ObpfTetrion const* const tetrion, ObpfVec2 const position) try {
    auto const pos = Vec2{ position.x, position.y };
    return static_cast<ObpfTetrominoType>(tetrion->matrix()[pos]);
} catch (std::exception const& e) {

    spdlog::error("Failed to get matrix value: {}", e.what());
    return OBPF_TETROMINO_TYPE_EMPTY;
} catch (...) {
    spdlog::error("Failed to get matrix value: Unknown error");
    return OBPF_TETROMINO_TYPE_EMPTY;
}

ObpfPreviewPieces obpf_tetrion_get_preview_pieces(ObpfTetrion const* tetrion) try {
    auto const preview_tetrominos = tetrion->get_preview_tetrominos();
    ObpfPreviewPieces result{};
    for (usize i = 0; i < preview_tetrominos.size(); ++i) {
        result.types[i] = static_cast<ObpfTetrominoType>(preview_tetrominos.at(i));
    }
    return result;
} catch (std::exception const& e) {

    spdlog::error("Failed to get preview pieces: {}", e.what());
    return ObpfPreviewPieces{};
} catch (...) {
    spdlog::error("Failed to get preview pieces: Unknown error");
    return ObpfPreviewPieces{};
}

ObpfMinoPositions obpf_tetromino_get_mino_positions(ObpfTetrominoType const type, ObpfRotation const rotation) try {
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
} catch (std::exception const& e) {

    spdlog::error("Failed to get mino positions: {}", e.what());
    return ObpfMinoPositions{};
} catch (...) {
    spdlog::error("Failed to get mino positions: Unknown error");
    return ObpfMinoPositions{};
}

ObpfTetrominoType obpf_tetrion_get_hold_piece(ObpfTetrion const* const tetrion) try {
    if (auto const hold_piece = tetrion->hold_piece(); hold_piece.has_value()) {
        return static_cast<ObpfTetrominoType>(hold_piece.value());
    }
    return OBPF_TETROMINO_TYPE_EMPTY;
} catch (std::exception const& e) {

    spdlog::error("Failed to get hold piece: {}", e.what());
    return OBPF_TETROMINO_TYPE_EMPTY;
} catch (...) {
    spdlog::error("Failed to get hold piece: Unknown error");
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

ObpfStats obpf_tetrion_get_stats(ObpfTetrion const* tetrion) try {
    return ObpfStats{
        .score = tetrion->score(),
        .lines_cleared = tetrion->num_lines_cleared(),
        .level = tetrion->level(),
    };
} catch (std::exception const& e) {

    spdlog::error("Failed to get stats: {}", e.what());
    return ObpfStats{};
} catch (...) {
    spdlog::error("Failed to get stats: Unknown error");
    return ObpfStats{};
}

bool obpf_tetrion_is_game_over(ObpfTetrion const* const tetrion) try {
    return tetrion->game_over_since_frame().has_value();
} catch (std::exception const& e) {

    spdlog::error("Failed to check if game is over: {}", e.what());
    return false;
} catch (...) {
    spdlog::error("Failed to check if game is over: Unknown error");
    return false;
}

void obpf_tetrion_set_action_handler(ObpfTetrion* const tetrion, ObpfActionHandler const handler, void* const user_data) try {
    tetrion->set_action_handler(handler, user_data);
} catch (std::exception const& e) {

    spdlog::error("Failed to set action handler: {}", e.what());
} catch (...) {
    spdlog::error("Failed to set action handler: Unknown error");
}

bool obpf_tetrion_is_connected(ObpfTetrion const* tetrion) {
    return tetrion->is_connected();
}

char const* obpf_tetrion_player_name(ObpfTetrion const* const tetrion) {
    return tetrion->player_name().c_str();
}

std::uint64_t obpf_tetrion_frames_until_game_start(ObpfTetrion const* tetrion) {
    return tetrion->frames_until_game_start();
}

std::uint64_t obpf_garbage_delay_frames() {
    return ObpfTetrion::garbage_delay_frames;
}
