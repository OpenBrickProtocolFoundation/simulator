include("${CMAKE_SOURCE_DIR}/cmake/CPM.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/system_link.cmake")

function(obpf_simulator_setup_dependencies)
    CPMAddPackage(
            NAME SPDLOG
            GITHUB_REPOSITORY gabime/spdlog
            VERSION 1.12.0
            OPTIONS
            "SPDLOG_BUILD_EXAMPLE OFF"
            "SPDLOG_BUILD_TESTS OFF"
    )
    CPMAddPackage(
            NAME GSL
            GITHUB_REPOSITORY microsoft/GSL
            VERSION 4.0.0
            OPTIONS
            "GSL_TEST OFF"
    )
    CPMAddPackage(
            NAME C2K_SOCKETS
            GITHUB_REPOSITORY mgerhold/sockets
            VERSION 0.4.0
            OPTIONS
            "GSL_TEST OFF"
    )
    CPMAddPackage(
            NAME TL_EXPECTED
            GITHUB_REPOSITORY TartanLlama/expected
            VERSION 1.1.0
            OPTIONS
            "EXPECTED_BUILD_PACKAGE OFF"
            "EXPECTED_BUILD_TESTS OFF"
            "EXPECTED_BUILD_PACKAGE_DEB OFF"
    )
endfunction()
