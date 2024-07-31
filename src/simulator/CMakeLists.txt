add_library(simulator STATIC
        include/simulator/bag.hpp
        include/simulator/input.hpp
        include/simulator/matrix.hpp
        include/simulator/rotation.hpp
        include/simulator/tetrion.hpp
        include/simulator/tetromino.hpp
        include/simulator/tetromino_type.hpp
        include/simulator/vec2.hpp
        tetrion.cpp
        tetromino.cpp
        include/simulator/wallkicks.hpp
        wallkicks.cpp
        include/simulator/delayed_auto_shift.hpp
        include/simulator/lock_delay.hpp
        include/simulator/entry_delay.hpp
        include/simulator/line_clear_delay.hpp
        include/simulator/key_state.hpp
        include/simulator/action.hpp
)

target_include_directories(simulator
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(simulator
        PRIVATE
        obpf_simulator_project_options
)

target_link_system_libraries(simulator
        PUBLIC
        Microsoft.GSL::GSL
        lib2k
        PRIVATE
        spdlog::spdlog
        tl::optional
        magic_enum::magic_enum
)
