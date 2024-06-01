 CPMAddPackage(
         NAME GTEST
         GITHUB_REPOSITORY google/googletest
         VERSION 1.14.0
         OPTIONS
         "INSTALL_GTEST OFF"
         "BUILD_SHARED_LIBS OFF"
 )

 add_executable(test_sandbox sandbox.cpp utils.hpp)
 target_link_libraries(test_sandbox PRIVATE obpf_simulator_project_options network)
 target_link_system_libraries(test_sandbox PRIVATE crapper)

 add_executable(simulator_tests network_tests.cpp utils.hpp)
 target_link_libraries(simulator_tests PRIVATE obpf_simulator_project_options network)
 target_link_system_libraries(simulator_tests
         PRIVATE
         gtest_main
         gmock_main
         Microsoft.GSL::GSL
         spdlog::spdlog
 )

 include(GoogleTest)
 gtest_discover_tests(simulator_tests)
