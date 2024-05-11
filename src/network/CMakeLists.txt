add_library(network
        include/network/message_types.hpp
        include/network/message_header.hpp
        include/network/messages.hpp
        messages.cpp
        include/network/detail/shared_lib_export.hpp
        include/network/constants.hpp
        include/network/user.hpp
        include/network/lobby_server.hpp
        lobby_server.cpp
        include/network/lobby.hpp
)

target_link_libraries(network PUBLIC simulator)
target_link_system_libraries(network
        PUBLIC
        c2k_sockets
        spdlog::spdlog
        tl::expected
        PRIVATE
        crapper
)
target_include_directories(network PUBLIC include)
