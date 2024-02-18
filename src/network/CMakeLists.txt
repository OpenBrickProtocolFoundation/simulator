add_library(network
        message_types.hpp
        message_header.hpp
        server.hpp
        server.cpp
        messages.hpp
        messages.cpp
)

target_link_libraries(network PRIVATE simulator)
target_link_system_libraries(network PRIVATE c2k_sockets)
