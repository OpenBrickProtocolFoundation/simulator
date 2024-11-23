#include <simulator/observer_tetrion.hpp>

#include "log_entry.hpp"

ObserverTetrion::ObserverTetrion(u64 const seed, u64 const start_frame, u8 const client_id, std::string player_name, Key)
    : ObpfTetrion{ seed, start_frame, std::move(player_name) }, m_client_id{ client_id } {}

[[nodiscard]] std::optional<GarbageSendEvent> ObserverTetrion::process_key_state(KeyState const key_state) {
    return ObpfTetrion::simulate_next_frame(key_state);
}
