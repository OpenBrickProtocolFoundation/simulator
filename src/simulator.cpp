#include "matrix.hpp"
#include "tetrion.hpp"
#include <memory>
#include <obpf/simulator.h>

Tetrion* obpf_create_tetrion() {
    return new Tetrion{};
}

bool obpf_tetrion_try_get_active_tetromino(Tetrion const* const tetrion, ObpfTetromino* const out_tetromino) {
    auto const active_tetromino = tetrion->active_tetromino();
    if (not active_tetromino.has_value()) {
        return false;
    }
    *out_tetromino = static_cast<ObpfTetromino>(active_tetromino.value());
    return true;
}

void obpf_tetrion_simulate_up_until(Tetrion* const tetrion, uint64_t const frame) {
    tetrion->simulate_up_until(frame);
}

void obpf_destroy_tetrion(Tetrion const* const tetrion) {
    delete tetrion;
}

Matrix const* obpf_tetrion_matrix(Tetrion const* const tetrion) {
    return std::addressof(tetrion->matrix());
}

ObpfTetrominoType obpf_matrix_get(Matrix const* const matrix, ObpfVec2 const position) {
    return static_cast<ObpfTetrominoType>((*matrix)[position]);
}
