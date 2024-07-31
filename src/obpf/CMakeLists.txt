include(GenerateExportHeader)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

set(CMAKE_DEBUG_POSTFIX "_d")

add_library(obpf
        include/obpf/constants.h
        include/obpf/input.h
        include/obpf/simulator.h
        include/obpf/tetromino.h
        include/obpf/vec2.h
        simulator.cpp
        include/obpf/lobby.h
        lobby.cpp
        include/obpf/rotation.h
        include/obpf/stats.h
        include/obpf/action.h
)

target_compile_definitions(obpf PRIVATE "simulator_EXPORTS")

generate_export_header(obpf)

target_link_libraries(obpf
        PRIVATE
        simulator
        network
        spdlog::spdlog
)

target_include_directories(obpf
        PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
        include
)
