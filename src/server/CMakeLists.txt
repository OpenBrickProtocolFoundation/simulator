add_executable(server
        server.hpp
        server.cpp
        main.cpp
)

target_include_directories(server PUBLIC include)
target_link_system_libraries(server
        PRIVATE
        network
        simulator
        lib2k
)
