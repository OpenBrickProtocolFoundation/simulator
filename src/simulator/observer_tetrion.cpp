#include <simulator/observer_tetrion.hpp>

[[nodiscard]] std::optional<GarbageSendEvent> ObserverTetrion::process_key_state(KeyState const key_state) {
    return ObpfTetrion::simulate_next_frame(key_state);
}
