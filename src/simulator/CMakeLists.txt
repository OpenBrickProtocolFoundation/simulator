set(CMAKE_DEBUG_POSTFIX "_d")

add_library(simulator
        include/obpf/simulator.h
        simulator.cpp
        include/obpf/tetromino_type.h
        matrix.hpp
        include/obpf/constants.h
        tetromino_type.hpp
        include/obpf/vec2.h
        vec2.hpp
        tetrion.cpp
        tetrion.hpp
        include/obpf/tetromino.h
        tetromino.hpp
        rotation.hpp
        tetromino.cpp
        include/obpf/input.h
)

target_include_directories(simulator
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_system_libraries(simulator
        PRIVATE
        Microsoft.GSL::GSL
        spdlog::spdlog
)

# Disable exceptions
if (NOT MSVC)
    target_compile_options(simulator PRIVATE -fno-exceptions)
endif ()
