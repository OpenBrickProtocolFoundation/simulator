 CPMAddPackage(
         NAME GTEST
         GITHUB_REPOSITORY google/googletest
         VERSION 1.14.0
         OPTIONS
         "INSTALL_GTEST OFF"
 )

 add_executable(simulator_tests network_tests.cpp)
 target_link_libraries(simulator_tests PRIVATE obpf_simulator_project_options network)
 target_link_system_libraries(simulator_tests PRIVATE gtest_main gmock_main)

 include(GoogleTest)
 gtest_discover_tests(simulator_tests)
