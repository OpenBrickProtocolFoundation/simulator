#include <chrono>
#include <cstdlib>
#include <simulator/multiplayer_tetrion.hpp>
#include <thread>

using namespace std::chrono_literals;

int main() {
    auto tetrion = MultiplayerTetrion::create("127.0.0.1", 12345);
    if (tetrion == nullptr) {
        return EXIT_FAILURE;
    }

    auto const end_time = std::chrono::steady_clock::now() + 5s;
    while (true) {
        if (std::chrono::steady_clock::now() >= end_time) {
            break;
        }
        tetrion->simulate_next_frame(KeyState{});
        std::this_thread::sleep_for(16ms);
    }
}
