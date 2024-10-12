add_library(network STATIC
        include/network/message_types.hpp
        include/network/message_header.hpp
        include/network/messages.hpp
        messages.cpp
        include/network/constants.hpp
        include/network/user.hpp
        include/network/lobby_server.hpp
        lobby_server.cpp
        include/network/lobby.hpp
)

target_link_libraries(network PUBLIC simulator obpf_simulator_project_options)
target_link_system_libraries(network
        PUBLIC
        c2k_sockets
        spdlog::spdlog
        tl::expected
        crapper
        magic_enum::magic_enum
)
target_include_directories(network PUBLIC include)
