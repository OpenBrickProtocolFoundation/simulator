#pragma once

#include <array>
#include <gsl/gsl>
#include <lib2k/types.hpp>
#include "tetromino_type.hpp"
#include "vec2.hpp"

class Matrix final {
public:
    static constexpr auto width = std::size_t{ 10 };
    static constexpr auto height = std::size_t{ 22 };
    static constexpr auto num_invisible_lines = std::size_t{ 2 };

private:
    std::array<TetrominoType, width * height> m_minos{};

public:
    void copy_line(std::size_t const destination, std::size_t const source) {
        for (auto column = std::size_t{ 0 }; column < width; ++column) {
            auto const destination_vec = Vec2{
                gsl::narrow<decltype(Vec2::x)>(column),
                gsl::narrow<decltype(Vec2::y)>(destination),
            };
            auto const source_vec = Vec2{
                gsl::narrow<decltype(Vec2::x)>(column),
                gsl::narrow<decltype(Vec2::y)>(source),
            };
            (*this)[destination_vec] = (*this)[source_vec];
        }
    }

    void fill(std::size_t const line, TetrominoType const type) {
        for (auto column = std::size_t{ 0 }; column < width; ++column) {
            auto const position = Vec2{
                gsl::narrow<decltype(Vec2::x)>(column),
                gsl::narrow<decltype(Vec2::y)>(line),
            };
            (*this)[position] = type;
        }
    }

    [[nodiscard]] bool is_line_full(std::size_t const line) const {
        for (auto column = std::size_t{ 0 }; column < width; ++column) {
            auto const position = Vec2{
                gsl::narrow<decltype(Vec2::x)>(column),
                gsl::narrow<decltype(Vec2::y)>(line),
            };
            if ((*this)[position] == TetrominoType::Empty) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] TetrominoType operator[](Vec2 const index) const {
        return m_minos.at(gsl::narrow<usize>(index.y) * width + gsl::narrow<usize>(index.x));
    }

    [[nodiscard]] TetrominoType& operator[](Vec2 const index) {
        return m_minos.at(gsl::narrow<usize>(index.y) * width + gsl::narrow<usize>(index.x));
    }
};
