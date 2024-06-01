#include <memory>
#include <obpf/simulator.h>
#include <simulator/matrix.hpp>
#include <simulator/tetrion.hpp>
#include <spdlog/spdlog.h>

ObpfTetrion* obpf_create_tetrion(uint64_t const seed) {
    return new ObpfTetrion{ seed };
}

bool obpf_tetrion_try_get_active_tetromino(ObpfTetrion const* const tetrion, ObpfTetromino* const out_tetromino) {
    auto const active_tetromino = tetrion->active_tetromino();
    if (not active_tetromino.has_value()) {
        return false;
    }
    auto const& mino_positions = get_mino_positions(active_tetromino.value());
    auto const result = ObpfTetromino{
        .mino_positions = {
            ObpfVec2{ mino_positions.at(0).x, mino_positions.at(0).y },
            ObpfVec2{ mino_positions.at(1).x, mino_positions.at(1).y },
            ObpfVec2{ mino_positions.at(2).x, mino_positions.at(2).y },
            ObpfVec2{ mino_positions.at(3).x, mino_positions.at(3).y },
        },
        .type = static_cast<ObpfTetrominoType>(active_tetromino->type),
    };
    *out_tetromino = result;
    return true;
}

void obpf_tetrion_simulate_up_until(ObpfTetrion* const tetrion, uint64_t const frame) {
    tetrion->simulate_up_until(frame);
}

void obpf_tetrion_enqueue_event(ObpfTetrion* const tetrion, ObpfEvent const event) {
    auto const e = Event{
        .key = static_cast<Key>(event.key),
        .type = static_cast<EventType>(event.type),
        .frame = event.frame,
    };
    tetrion->enqueue_event(e);
}

void obpf_destroy_tetrion(ObpfTetrion const* const tetrion) {
    delete tetrion;
}

ObpfMatrix const* obpf_tetrion_matrix(ObpfTetrion const* const tetrion) {
    return reinterpret_cast<ObpfMatrix const*>(std::addressof(tetrion->matrix()));
}

ObpfTetrominoType obpf_matrix_get(ObpfMatrix const* const matrix, ObpfVec2 const position) {
    auto const pos = Vec2{ position.x, position.y };
    return static_cast<ObpfTetrominoType>((*reinterpret_cast<Matrix const*>(matrix))[pos]);
}

uint8_t obpf_tetrion_width() {
    return uint8_t{ Matrix::width };
}

uint8_t obpf_tetrion_height() {
    return uint8_t{ Matrix::height };
}
