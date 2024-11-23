#include <spdlog/spdlog.h>
#include <algorithm>
#include <chrono>
#include <deque>
#include <lib2k/types.hpp>
#include <simulator/multiplayer_tetrion.hpp>
#include <unordered_map>

class Move {
    bool m_hold;
    int m_rotation;
    int m_movement;
    bool m_dropped = false;

public:
    Move(bool const hold, int const rotation, int const movement)
        : m_hold{ hold }, m_rotation{ rotation }, m_movement{ movement } {}

    [[nodiscard]] std::optional<Key> next_key() {
        if (m_hold) {
            m_hold = false;
            return Key::Hold;
        }
        if (m_rotation > 0) {
            --m_rotation;
            return Key::RotateClockwise;
        }
        if (m_movement < 0) {
            ++m_movement;
            return Key::Left;
        }
        if (m_movement > 0) {
            --m_movement;
            return Key::Right;
        }
        if (not m_dropped) {
            m_dropped = true;
            return Key::Drop;
        }
        return std::nullopt;
    }
};

class MoveChain {
    std::deque<Move> m_moves;

public:
    MoveChain(std::initializer_list<Move> const moves)
        : m_moves{ moves } {}

    template<typename... Args>
    void emplace_front(Args&&... args) {
        m_moves.emplace_back(std::forward<Args>(args)...);
    }

    [[nodiscard]] std::optional<Key> next_key() {
        while (not m_moves.empty()) {
            auto const key = m_moves.front().next_key();
            if (key.has_value()) {
                return key;
            }
            m_moves.pop_front();
        }
        return std::nullopt;
    }
};

[[nodiscard]] KeyState to_key_state(Key const key) {
    return KeyState{
        key == Key::Left, key == Key::Right,           key == Key::Down,
        key == Key::Drop, key == Key::RotateClockwise, key == Key::RotateCounterClockwise,
        key == Key::Hold,
    };
}

[[nodiscard]] int determine_score(ObpfTetrion const& tetrion) {
    auto score = static_cast<int>(Matrix::height);
    for (auto row = decltype(Matrix::height){ 0 }; row < Matrix::height; ++row) {
        for (auto column = decltype(Matrix::width){ 0 }; column < Matrix::width; ++column) {
            if (tetrion.matrix()[Vec2{ static_cast<i32>(column), static_cast<i32>(row) }] != TetrominoType::Empty) {
                score = static_cast<decltype(score)>(row);
                goto determined_row;
            }
        }
    }
determined_row:

    for (auto y = decltype(Matrix::height){ 0 }; y < Matrix::height - 1; ++y) {
        auto const row = Matrix::height - y - 1;
        for (auto column = decltype(Matrix::width){ 0 }; column < Matrix::width; ++column) {
            if (tetrion.matrix()[Vec2{ static_cast<i32>(column), static_cast<i32>(row) }] == TetrominoType::Empty
                and tetrion.matrix()[Vec2{ static_cast<i32>(column), static_cast<i32>(row) - 1 }]
                        != TetrominoType::Empty) {
                score -= 6;
            }
        }
    }

    for (auto row = decltype(Matrix::height){ 0 }; row < Matrix::height; ++row) {
        auto empty_cells = 0;
        for (auto column = decltype(Matrix::width){ 0 }; column < Matrix::width; ++column) {
            if (tetrion.matrix()[Vec2{ static_cast<i32>(column), static_cast<i32>(row) }] == TetrominoType::Empty) {
                ++empty_cells;
            }
        }
        if (empty_cells < static_cast<decltype(empty_cells)>(Matrix::width) and empty_cells > 1) {
            --score;
        }
    }

    return score;
}

struct Heuristic {
    MoveChain moves;
    int score;
};

[[nodiscard]] Heuristic determine_next_move_chain(ObpfTetrion const& tetrion, int const lookahead) {
    if (not tetrion.active_tetromino().has_value()) {
        // We're still in the countdown phase or during lock delay (or some other delay).
        return {};
    }
    auto heuristics = std::vector<Heuristic>{};
    for (auto hold : std::array{ true, false }) {
        for (auto movement = -5; movement <= 5; ++movement) {
            for (auto rotation = 0; rotation < 4; ++rotation) {
                auto copy = ObpfTetrion{ tetrion };

                if (hold) {
                    std::ignore = copy.simulate_next_frame(to_key_state(Key::Hold));
                    std::ignore = copy.simulate_next_frame(KeyState{});
                }

                for (auto i = 0; i < rotation; ++i) {
                    std::ignore = copy.simulate_next_frame(to_key_state(Key::RotateClockwise));
                    std::ignore = copy.simulate_next_frame(KeyState{});
                }

                for (auto i = 0; i > movement; --i) {
                    std::ignore = copy.simulate_next_frame(to_key_state(Key::Left));
                    std::ignore = copy.simulate_next_frame(KeyState{});
                }
                for (auto i = 0; i < movement; ++i) {
                    std::ignore = copy.simulate_next_frame(to_key_state(Key::Right));
                    std::ignore = copy.simulate_next_frame(KeyState{});
                }
                std::ignore = copy.simulate_next_frame(to_key_state(Key::Drop));
                std::ignore = copy.simulate_next_frame(KeyState{});

                // Await lock delay.
                while (not copy.active_tetromino().has_value()) {
                    if (copy.game_over_since_frame().has_value()) {
                        // We've lost the game.
                        return {};
                    }
                    std::ignore = copy.simulate_next_frame(KeyState{});
                }

                if (lookahead > 0) {
                    auto best_move = determine_next_move_chain(copy, lookahead - 1);
                    best_move.moves.emplace_front(Move{ hold, rotation, movement });
                    heuristics.push_back(std::move(best_move));
                } else {
                    heuristics.emplace_back(
                        MoveChain{
                            Move{ hold, rotation, movement }
                    },
                        determine_score(copy)
                    );
                }
                if (tetrion.active_tetromino().value().type == TetrominoType::O) {
                    // No sense in rotating the O piece.
                    break;
                }
            }
        }
    }

    return *std::ranges::max_element(heuristics, [](auto const& lhs, auto const& rhs) { return lhs.score < rhs.score; });
}

int main() {
    using namespace std::chrono_literals;
    auto tetrion = MultiplayerTetrion::create("127.0.0.1", 12345, "bot");
    auto last_tick = std::chrono::high_resolution_clock::now();
    static constexpr auto ticks_per_second = 60;
    static constexpr auto tick_duration = decltype(last_tick)::duration{ 1s } / ticks_per_second;
    spdlog::info("tick duration: {}", tick_duration.count());
    auto tick = u64{ 0 };
    auto current_move_chain = determine_next_move_chain(*tetrion, 0);
    auto made_input_during_last_tick = false;

    while (not tetrion->game_over_since_frame().has_value()) {
        while (std::chrono::high_resolution_clock::now() - last_tick >= tick_duration) {
            ++tick;
            spdlog::info("simulating tick {}", tick);
            if (made_input_during_last_tick or not tetrion->active_tetromino().has_value()) {
                // Release keys.
                std::ignore = tetrion->simulate_next_frame(KeyState{});
                made_input_during_last_tick = false;
            } else if (tetrion->active_tetromino().has_value()) {
                auto const next_key = current_move_chain.moves.next_key();
                if (not next_key.has_value()) {
                    std::ignore = tetrion->simulate_next_frame(KeyState{});
                    current_move_chain = determine_next_move_chain(*tetrion, 0);
                    auto const key = current_move_chain.moves.next_key();
                    if (key.has_value()) {
                        std::ignore = tetrion->simulate_next_frame(to_key_state(key.value()));
                        made_input_during_last_tick = true;
                    }
                } else {
                    spdlog::info("sending input");
                    std::ignore = tetrion->simulate_next_frame(to_key_state(next_key.value()));
                    made_input_during_last_tick = true;
                }
            }
            last_tick += tick_duration;
        }
    }
}
