include("${CMAKE_SOURCE_DIR}/cmake/CPM.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/system_link.cmake")

function(setup_dependencies)
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
endfunction()
