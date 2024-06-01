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
)

target_compile_definitions(obpf PRIVATE "simulator_EXPORTS")

generate_export_header(obpf)

target_link_libraries(obpf
        PRIVATE
        simulator
        spdlog::spdlog
)

target_include_directories(obpf
        PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
        include
)
