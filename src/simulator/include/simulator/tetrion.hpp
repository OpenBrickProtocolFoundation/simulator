#pragma once

#include "bag.hpp"
#include "input.hpp"
#include "matrix.hpp"
#include "tetromino.hpp"
#include <cstdint>
#include <lib2k/random.hpp>
#include <lib2k/types.hpp>
#include <optional>
#include <vector>

struct ObpfTetrion final {
private:
    static constexpr auto spawn_position = Vec2{ 3, 0 };
    static constexpr auto spawn_rotation = Rotation::North;

    Matrix m_matrix;
    std::optional<Tetromino> m_active_tetromino;
    std::uint64_t m_next_frame = 0;
    std::vector<Event> m_events;
    c2k::Random m_random;
    std::array<Bag, 2> m_bags;
    usize m_bag_index = 0;

public:
    explicit ObpfTetrion(std::uint64_t const seed) : m_random{ seed }, m_bags{ create_two_bags(m_random) } {
        static_assert(std::same_as<std::remove_const_t<decltype(seed)>, c2k::Random::Seed>);
        spawn_next_tetromino();
    }

    [[nodiscard]] Matrix const& matrix() const {
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
    void handle_keypress(Key key);
    void move_left();
    void move_right();
    void move_down();
    void rotate_clockwise();
    void rotate_counter_clockwise();
    void drop();
    [[nodiscard]] static std::array<Bag, 2> create_two_bags(c2k::Random& random);
};
