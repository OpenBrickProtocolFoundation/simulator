cmake_minimum_required(VERSION 3.27)
project(obpf_simulator)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (MSVC)
    string(APPEND CMAKE_CXX_FLAGS " /EHs")
endif ()

include(dependencies.cmake)
obpf_simulator_setup_dependencies()

include(project_options.cmake)

add_subdirectory(src bin)

if (${obpf_build_tests})
    add_subdirectory(test)
endif ()
