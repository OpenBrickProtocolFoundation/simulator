#pragma once

#include <array>
#include <cstdint>
#include <lib2k/random.hpp>
#include <lib2k/static_vector.hpp>
#include <lib2k/types.hpp>
#include <optional>
#include <vector>
#include "bag.hpp"
#include "delayed_auto_shift.hpp"
#include "entry_delay.hpp"
#include "input.hpp"
#include "line_clear_delay.hpp"
#include "lock_delay.hpp"
#include "matrix.hpp"
#include "tetromino.hpp"

struct ObpfTetrion final {
private:
    static constexpr auto spawn_position = Vec2{ 3, 0 };
    static constexpr auto spawn_rotation = Rotation::North;

    Matrix m_matrix;
    std::optional<Tetromino> m_active_tetromino;
    std::optional<Tetromino> m_ghost_tetromino;
    std::optional<TetrominoType> m_hold_piece;
    std::optional<TetrominoType> m_old_hold_piece;
    bool m_is_hold_possible = true;
    u64 m_next_frame = 0;
    std::vector<Event> m_events;
    c2k::Random m_random;
    std::array<Bag, 2> m_bags;
    usize m_bag_index = 0;
    DelayedAutoShiftState m_auto_shift_state;
    LockDelayState m_lock_delay_state;
    EntryDelay m_entry_delay;
    LineClearDelay m_line_clear_delay;
    u32 m_lines_cleared = 0;
    u64 m_next_gravity_frame = gravity_delay_by_level(0);  // todo: offset by starting frame given by the server
    bool m_is_soft_dropping = false;

    static constexpr u64 gravity_delay_by_level(u32 const level) {
        constexpr auto delays = std::array<u64, 13>{
            60, 48, 37, 28, 21, 16, 11, 8, 6, 4, 3, 2, 1,
        };
        return delays.at(std::min(static_cast<usize>(level), delays.size() - 1));
    }

public:
    enum class DownMovementType {
        Gravity,
        SoftDrop,
    };

    explicit ObpfTetrion(std::uint64_t const seed)
        : m_random{ seed }, m_bags{ create_two_bags(m_random) } {
        static_assert(std::same_as<std::remove_const_t<decltype(seed)>, c2k::Random::Seed>);
        spawn_next_tetromino();
    }

    [[nodiscard]] Matrix const& matrix() const {
        return m_matrix;
    }

    [[nodiscard]] std::optional<Tetromino> active_tetromino() const {
        return m_active_tetromino;
    }

    [[nodiscard]] std::optional<Tetromino> ghost_tetromino() const {
        return m_ghost_tetromino;
    }

    void simulate_up_until(std::uint64_t frame);
    void enqueue_event(Event const& event);
    [[nodiscard]] LineClearDelay::State line_clear_delay_state() const;
    [[nodiscard]] std::array<TetrominoType, 6> get_preview_tetrominos() const;
    [[nodiscard]] std::optional<TetrominoType> hold_piece() const;

private:
    void freeze_and_destroy_active_tetromino();
    [[nodiscard]] bool is_tetromino_position_valid(Tetromino const& tetromino) const;
    [[nodiscard]] bool is_active_tetromino_position_valid() const;
    void spawn_next_tetromino();
    void process_events();
    void discard_events(u64 frame);
    void handle_key_press(Key key);
    void handle_key_release(Key key);
    void move_left();
    void move_right();
    void move_down(DownMovementType movement_type);
    void rotate(RotationDirection direction);
    void rotate_clockwise();
    void rotate_counter_clockwise();
    void drop();
    void hold();
    void determine_lines_to_clear();
    void clear_lines(c2k::StaticVector<u8, 4> lines);
    [[nodiscard]] u32 level() const;
    void refresh_ghost_tetromino();

    [[nodiscard]] static std::array<Bag, 2> create_two_bags(c2k::Random& random);
};
