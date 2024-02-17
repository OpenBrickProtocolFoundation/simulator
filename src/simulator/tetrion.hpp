#pragma once

#include "matrix.hpp"
#include "tetromino.hpp"
#include <cstdint>
#include <obpf/input.h>
#include <optional>
#include <vector>

struct ObpfTetrion final {
private:
    using Event = ObpfEvent;

    static constexpr auto spawn_position = Vec2{ 3, 0 };
    static constexpr auto spawn_rotation = Rotation::North;

    ObpfMatrix m_matrix;
    std::optional<Tetromino> m_active_tetromino;
    std::uint64_t m_next_frame = 0;
    std::vector<Event> m_events;

public:
    ObpfTetrion() {
        spawn_next_tetromino();
    }

    [[nodiscard]] ObpfMatrix const& matrix() const {
        return m_matrix;
    }

    [[nodiscard]] std::optional<Tetromino> active_tetromino() const {
        return m_active_tetromino;
    }

    void simulate_up_until(std::uint64_t frame);
    void enqueue_event(Event const& event);

private:
    void freeze_and_destroy_active_tetromino();
    [[nodiscard]] bool is_active_tetromino_position_valid() const;
    void spawn_next_tetromino();
    void process_events();
    void handle_keypress(ObpfKey key);
    void move_left();
    void move_right();
    void move_down();
    void drop();
};
