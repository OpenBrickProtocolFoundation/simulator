#pragma once

#include <array>
#include <filesystem>
#include <lib2k/types.hpp>
#include <ostream>
#include <simulator/key_state.hpp>
#include <simulator/matrix.hpp>
#include <simulator/tetromino.hpp>
#include <sockets/detail/message_buffer.hpp>
#include <vector>
#include "utils.hpp"

enum class LogEntryType : u8 {
    LogEntry,
    LogEvent,
};

struct ObpfTetrion;

struct LogTetromino {
    i32 x;
    i32 y;
    Rotation rotation;
    TetrominoType type;

    LogTetromino(i32 const x, i32 const y, Rotation const rotation, TetrominoType const type)
        : x{ x }, y{ y }, rotation{ rotation }, type{ type } {}

    [[nodiscard]] constexpr bool operator==(LogTetromino const& other) const = default;

    [[nodiscard]] static LogTetromino null() {
        return LogTetromino{ -1, -1, Rotation::East, TetrominoType::Empty };
    }

    friend c2k::MessageBuffer& operator<<(c2k::MessageBuffer& buffer, LogTetromino const& tetromino) {
        return buffer << tetromino.x << tetromino.y << static_cast<u8>(tetromino.rotation)
                      << static_cast<u8>(tetromino.type);
    }
};

struct LogKeyState {
    bool left;
    bool right;
    bool down;
    bool drop;
    bool rotate_cw;
    bool rotate_ccw;
    bool hold;

    explicit LogKeyState(
        bool const left,
        bool const right,
        bool const down,
        bool const drop,
        bool const rotate_cw,
        bool const rotate_ccw,
        bool const hold
    )
        : left{ left },
          right{ right },
          down{ down },
          drop{ drop },
          rotate_cw{ rotate_cw },
          rotate_ccw{ rotate_ccw },
          hold{ hold } {}

    friend c2k::MessageBuffer& operator<<(c2k::MessageBuffer& buffer, LogKeyState const& state) {
        return buffer << static_cast<u8>(state.left) << static_cast<u8>(state.right) << static_cast<u8>(state.down)
                      << static_cast<u8>(state.drop) << static_cast<u8>(state.rotate_cw)
                      << static_cast<u8>(state.rotate_ccw) << static_cast<u8>(state.hold);
    }
};

struct LogGarbageSendEvent {
    u64 frame = 0;
    u8 num_lines = 0;

    LogGarbageSendEvent() = default;

    explicit LogGarbageSendEvent(u64 const frame, u8 const num_lines)
        : frame{ frame }, num_lines{ num_lines } {}

    friend c2k::MessageBuffer& operator<<(c2k::MessageBuffer& buffer, LogGarbageSendEvent const event) {
        return buffer << event.frame << event.num_lines;
    }
};

struct LogEntry {
    u8 client_id;
    std::array<std::array<TetrominoType, Matrix::width>, Matrix::height> matrix;
    LogTetromino active_tetromino;
    TetrominoType hold_piece;
    u64 next_frame;
    LogKeyState key_state;
    u64 game_over_since_frame;
    std::vector<LogGarbageSendEvent> m_incoming_garbage;
    std::string player_name;

    explicit LogEntry(ObpfTetrion const& tetrion, KeyState current_key_state);

    explicit LogEntry(
        u8 const client_id,
        std::array<std::array<TetrominoType, Matrix::width>, Matrix::height> const& matrix,
        LogTetromino const active_tetromino,
        TetrominoType const hold_piece,
        u64 const next_frame,
        LogKeyState const key_state,
        u64 const game_over_since_frame,
        std::vector<LogGarbageSendEvent> garbage_send_queue,
        std::string player_name
    )
        : client_id{ client_id },
          matrix{ matrix },
          active_tetromino{ active_tetromino },
          hold_piece{ hold_piece },
          next_frame{ next_frame },
          key_state{ key_state },
          game_over_since_frame{ game_over_since_frame },
          m_incoming_garbage{ std::move(garbage_send_queue) },
          player_name{ std::move(player_name) } {}

    friend std::ostream& operator<<(std::ostream& stream, LogEntry const& entry) {
        auto buffer = c2k::MessageBuffer{};
        buffer << std::to_underlying(LogEntryType::LogEntry) << entry.client_id;
        for (auto const& row : entry.matrix) {
            for (auto const mino : row) {
                buffer << static_cast<u8>(mino);
            }
        }
        buffer << entry.active_tetromino << static_cast<u8>(entry.hold_piece) << entry.next_frame << entry.key_state
               << entry.game_over_since_frame << entry.m_incoming_garbage.size();
        for (auto const event : entry.m_incoming_garbage) {
            buffer << event;
        }
        buffer << gsl::narrow<u8>(entry.player_name.size());
        for (auto const c : entry.player_name) {
            buffer << c;
        }
        stream.write(
            reinterpret_cast<char const*>(buffer.data().data()),
            gsl::narrow<std::streamsize>(buffer.data().size())
        );
        return stream;
    }
};

[[nodiscard]] inline std::filesystem::path generate_log_filepath(u8 const client_id, std::string_view const player_name) {
    return std::format("logs/{}-{}-{}.log", get_current_date_time(), client_id, player_name);
}

void ensure_path(std::filesystem::path const& path);
void create_empty_file(std::filesystem::path const& path);

struct LogEvent final {
    enum class Type : u8 {
        SendingGarbage,
    };

    u8 client_id;
    u64 next_frame;
    Type type;

    friend std::ostream& operator<<(std::ostream& stream, LogEvent const& entry) {
        auto buffer = c2k::MessageBuffer{};
        buffer << std::to_underlying(LogEntryType::LogEvent) << entry.client_id << entry.next_frame
               << std::to_underlying(entry.type);
        stream.write(
            reinterpret_cast<char const*>(buffer.data().data()),
            gsl::narrow<std::streamsize>(buffer.data().size())
        );
        return stream;
    }
};
