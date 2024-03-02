include(${PROJECT_SOURCE_DIR}/cmake/warnings.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/sanitizers.cmake)

# the following function was taken from:
# https://github.com/cpp-best-practices/cmake_template/blob/main/ProjectOptions.cmake
macro(check_sanitizer_support)
    if ((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND NOT WIN32)
        set(supports_ubsan ON)
    else ()
        set(supports_ubsan OFF)
    endif ()

    if ((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND WIN32)
        set(supports_asan OFF)
    else ()
        set(supports_asan ON)
    endif ()
endmacro()

if (PROJECT_IS_TOP_LEVEL)
    option(obpf_simulator_warnings_as_errors "Treat warnings as errors" ON)
    option(obpf_simulator_enable_undefined_behavior_sanitizer "Enable undefined behavior sanitizer" ${supports_ubsan})
    option(obpf_simulator_enable_address_sanitizer "Enable address sanitizer" ${supports_asan})
    option(obpf_build_tests "Build unit tests" ON)
else ()
    option(obpf_simulator_warnings_as_errors "Treat warnings as errors" OFF)
    option(obpf_simulator_enable_undefined_behavior_sanitizer "Enable undefined behavior sanitizer" OFF)
    option(obpf_simulator_enable_address_sanitizer "Enable address sanitizer" OFF)
    option(obpf_build_tests "Build unit tests" OFF)
endif ()
option(obpf_simulator_build_shared_libs "Build shared libraries instead of static libraries" ON)
set(BUILD_SHARED_LIBS ${obpf_simulator_build_shared_libs})

add_library(obpf_simulator_warnings INTERFACE)
obpf_simulator_set_warnings(obpf_simulator_warnings ${obpf_simulator_warnings_as_errors})

add_library(obpf_simulator_sanitizers INTERFACE)
obpf_simulator_enable_sanitizers(obpf_simulator_sanitizers ${obpf_simulator_enable_address_sanitizer} ${obpf_simulator_enable_undefined_behavior_sanitizer})

add_library(obpf_simulator_project_options INTERFACE)
target_link_libraries(obpf_simulator_project_options
        INTERFACE obpf_simulator_warnings
        INTERFACE obpf_simulator_sanitizers
)
