#include <cassert>
#include <map>
#include <ranges>
#include <simulator/garbage.hpp>
#include <simulator/tetrion.hpp>

[[nodiscard]] tl::optional<ObpfTetrion&> determine_garbage_target(
    std::vector<ObpfTetrion*> const& tetrions,
    u8 const sender_tetrion_id,
    u64 const frame
) {
    if (tetrions.size() < 2) {
        assert(tetrions.empty() or tetrions.front()->id() == sender_tetrion_id);
        return tl::nullopt;
    }

    auto alive_tetrions = std::map<u8, ObpfTetrion*>{};
    for (auto const tetrion : tetrions) {
        if (tetrion->id() == sender_tetrion_id) {
            continue;
        }
        if (auto const game_over_since_frame = tetrion->game_over_since_frame();
            game_over_since_frame.has_value() and frame >= game_over_since_frame.value()) {
            continue;
        }
        alive_tetrions[tetrion->id()] = tetrion;
    }
    if (alive_tetrions.empty()) {
        return tl::nullopt;
    }
    auto const find_iterator = std::ranges::find_if(alive_tetrions, [sender_tetrion_id](auto const& pair) {
        auto const id = pair.first;
        return id > sender_tetrion_id;
    });
    if (find_iterator != alive_tetrions.end()) {
        return *(find_iterator->second);
    }
    assert(alive_tetrions.begin()->first != sender_tetrion_id);
    return *(alive_tetrions.begin()->second);
}
