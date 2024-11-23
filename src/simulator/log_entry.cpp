#include "log_entry.hpp"
#include <format>
#include <fstream>
#include <simulator/tetrion.hpp>
#include <stdexcept>

[[nodiscard]] static auto get_matrix(ObpfTetrion const& tetrion) {
    auto matrix = decltype(LogEntry::matrix){};
    for (auto y = usize{ 0 }; y < matrix.size(); ++y) {
        for (auto x = usize{ 0 }; x < matrix.at(y).size(); ++x) {
            matrix.at(y).at(x) = tetrion.matrix()[Vec2{ static_cast<i32>(x), static_cast<i32>(y) }];
        }
    }
    return matrix;
}

[[nodiscard]] static LogTetromino get_active_tetromino(ObpfTetrion const& tetrion) {
    return tetrion.active_tetromino()
        .transform([&](Tetromino const& tetromino) {
            return LogTetromino{
                tetromino.position.x,
                tetromino.position.y,
                tetromino.rotation,
                tetromino.type,
            };
        })
        .value_or(LogTetromino::null());
}

[[nodiscard]] static LogKeyState get_key_state(KeyState const key_state) {
    // clang-format off
    return LogKeyState{
        key_state.get(Key::Left),
        key_state.get(Key::Right),
        key_state.get(Key::Down),
        key_state.get(Key::Drop),
        key_state.get(Key::RotateClockwise),
        key_state.get(Key::RotateCounterClockwise),
        key_state.get(Key::Hold),
    };
    // clang-format on
}

[[nodiscard]] static auto get_garbage_queue(ObpfTetrion const& tetrion) {
    auto garbage_send_queue = std::vector<LogGarbageSendEvent>{};
    garbage_send_queue.reserve(tetrion.garbage_queue_num_events());
    for (auto i = usize{ 0 }; i < tetrion.garbage_queue_num_events(); ++i) {
        auto const& event = tetrion.garbage_queue_event(i);
        garbage_send_queue.emplace_back(event.frame, event.num_lines);
    }
    return garbage_send_queue;
}

LogEntry::LogEntry(ObpfTetrion const& tetrion, KeyState const current_key_state)
    : client_id{ tetrion.id() },
      matrix{ get_matrix(tetrion) },
      active_tetromino{ get_active_tetromino(tetrion) },
      hold_piece{ tetrion.hold_piece().value_or(TetrominoType::Empty) },
      next_frame{ tetrion.next_frame() },
      key_state{ get_key_state(current_key_state) },
      game_over_since_frame{ tetrion.game_over_since_frame().value_or(0) },
      garbage_send_queue{ get_garbage_queue(tetrion) },
      player_name{ tetrion.player_name() } {}

void ensure_path(std::filesystem::path const& path) {
    if (path.has_parent_path()) {
        create_directories(path.parent_path());
    }
}

void create_empty_file(std::filesystem::path const& path) {
    ensure_path(path);
    if (not std::ofstream{ path, std::ios::out | std::ios::binary }) {
        throw std::runtime_error{ std::format("Unable to create new empty file at '{}'.", path.string()) };
    }
}
