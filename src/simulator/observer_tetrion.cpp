#include <simulator/observer_tetrion.hpp>

void ObserverTetrion::process_key_state(KeyState const key_state) {
    ObpfTetrion::simulate_next_frame(key_state);
}
