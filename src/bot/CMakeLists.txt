add_executable(bot main.cpp)

target_link_libraries(
        bot
        PRIVATE
        obpf_simulator_project_options
        simulator
        common
)

target_link_system_libraries(bot
        PRIVATE
        spdlog::spdlog
)
