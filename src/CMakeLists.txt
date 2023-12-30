set(SOURCE_FILES
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

set(CMAKE_DEBUG_POSTFIX "_d")

add_library(simulator_static STATIC ${SOURCE_FILES})
add_library(simulator_dynamic SHARED ${SOURCE_FILES})

set(TARGET_LIST simulator_static simulator_dynamic)

foreach (TARGET ${TARGET_LIST})
    target_include_directories(${TARGET}
            PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_link_system_libraries(${TARGET}
            PRIVATE
            Microsoft.GSL::GSL
            spdlog::spdlog
    )

    # Disable exceptions
    if(NOT MSVC)
        target_compile_options(${TARGET} PRIVATE -fno-exceptions)
    endif()
endforeach ()
