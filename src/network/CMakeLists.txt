add_library(network STATIC
        include/network/message_types.hpp
        include/network/message_header.hpp
        include/network/messages.hpp
        messages.cpp
        include/network/constants.hpp
)

target_link_libraries(network PUBLIC simulator obpf_simulator_project_options)
target_link_system_libraries(network
        PUBLIC
        c2k_sockets
        spdlog::spdlog
        tl::expected
        magic_enum::magic_enum
)
target_include_directories(network PUBLIC include)
