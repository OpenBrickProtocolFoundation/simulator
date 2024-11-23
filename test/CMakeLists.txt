 CPMAddPackage(
         NAME GTEST
         GITHUB_REPOSITORY google/googletest
         VERSION 1.15.2
         OPTIONS
         "INSTALL_GTEST OFF"
         "BUILD_SHARED_LIBS OFF"
 )

 add_executable(sandbox2 sandbox2.cpp)
 target_link_libraries(sandbox2 PRIVATE obpf_simulator_project_options network simulator)

 add_executable(
         simulator_tests
         network_tests.cpp
         utils.hpp
         tetrion_tests.cpp
 )
 target_link_libraries(
         simulator_tests
         PRIVATE
         simulator
         obpf_simulator_project_options
         network
 )
 target_link_system_libraries(simulator_tests
         PRIVATE
         gtest_main
         gmock_main
         Microsoft.GSL::GSL
         spdlog::spdlog
 )

 include(GoogleTest)
 gtest_discover_tests(simulator_tests)
