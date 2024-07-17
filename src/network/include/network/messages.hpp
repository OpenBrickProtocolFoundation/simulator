#pragma once

#include "constants.hpp"
#include "message_header.hpp"
#include "message_types.hpp"
#include <array>
#include <memory>
#include <simulator/input.hpp>
#include <simulator/matrix.hpp>
#include <simulator/tetromino_type.hpp>
#include <sockets/sockets.hpp>
#include <spdlog/spdlog.h>
#include <unordered_set>
#include <vector>

class EventDeserializationError final : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class MessageDeserializationError final : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class MessageInstantiationError final : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

[[nodiscard]] Event deserialize_event(c2k::MessageBuffer& buffer);


struct AbstractMessage {
    virtual ~AbstractMessage() = default;

    [[nodiscard]] bool operator==(AbstractMessage const& other) const {
        return typeid(*this) == typeid(other) and equals(other);
    }

    [[nodiscard]] virtual MessageType type() const = 0;
    [[nodiscard]] virtual decltype(MessageHeader::payload_size) payload_size() const = 0;
    [[nodiscard]] virtual c2k::MessageBuffer serialize() const = 0;

    // clang-format off
    [[nodiscard]] static std::unique_ptr<AbstractMessage> from_socket(
        c2k::ClientSocket& socket,
        std::chrono::steady_clock::duration timeout = std::chrono::seconds{ 2 }
    );
    // clang-format on

private:
    [[nodiscard]] virtual bool equals(AbstractMessage const& other) const = 0;
};

struct Heartbeat final : AbstractMessage {
    std::uint64_t frame;
    std::vector<Event> events; // at least 0, at most heartbeat_interval

    Heartbeat(std::uint64_t const frame, std::vector<Event> events) : frame{ frame }, events{ std::move(events) } { }

    [[nodiscard]] MessageType type() const override;
    [[nodiscard]] decltype(MessageHeader::payload_size) payload_size() const override;
    [[nodiscard]] c2k::MessageBuffer serialize() const override;
    [[nodiscard]] static Heartbeat deserialize(c2k::MessageBuffer& buffer);

    [[nodiscard]] static constexpr decltype(MessageHeader::payload_size) max_payload_size() {
        return calculate_payload_size(heartbeat_interval * 4);  // allow up to 4 events per frame
    }

private:
    [[nodiscard]] static constexpr decltype(MessageHeader::payload_size) calculate_payload_size(
            std::size_t const num_events
    ) {
        // clang-format off
        return static_cast<decltype(MessageHeader::payload_size)>(
            sizeof(frame)
            + sizeof(std::uint8_t) // number of events
            + num_events * (
                sizeof(std::uint8_t)    // key
                + sizeof(std::uint8_t)  // type
                + sizeof(std::uint64_t) // frame
            )
        );
        // clang-format on
    }

    [[nodiscard]] bool equals(AbstractMessage const& other) const override {
        auto const& other_heartbeat = static_cast<decltype(*this)&>(other);
        return std::tie(frame, events) == std::tie(other_heartbeat.frame, other_heartbeat.events);
    }
};

struct GridState final : AbstractMessage {
    std::uint64_t frame;
    std::array<TetrominoType, Matrix::width * Matrix::height> grid_contents;

    GridState(std::uint64_t const frame, std::array<TetrominoType, 10 * 22> const& grid_contents)
        : frame{ frame },
          grid_contents{ grid_contents } { }

    [[nodiscard]] MessageType type() const override;
    [[nodiscard]] decltype(MessageHeader::payload_size) payload_size() const override;
    [[nodiscard]] c2k::MessageBuffer serialize() const override;
    [[nodiscard]] static GridState deserialize(c2k::MessageBuffer& buffer);

    [[nodiscard]] static constexpr decltype(MessageHeader::payload_size) max_payload_size() {
        return calculate_payload_size();
    }

private:
    [[nodiscard]] static constexpr decltype(MessageHeader::payload_size) calculate_payload_size() {
        return sizeof(frame) + std::tuple_size_v<decltype(grid_contents)> * sizeof(std::uint8_t);
    }

    [[nodiscard]] bool equals(AbstractMessage const& other) const override {
        auto const& other_gridstate = static_cast<decltype(*this)&>(other);
        return std::tie(frame, grid_contents) == std::tie(other_gridstate.frame, other_gridstate.grid_contents);
    }
};

struct GameStart final : AbstractMessage {
    std::uint8_t client_id;
    std::uint64_t start_frame;
    std::uint64_t random_seed;

    GameStart(std::uint8_t const client_id, std::uint64_t const start_frame, std::uint64_t const random_seed)
        : client_id{ client_id },
          start_frame{ start_frame },
          random_seed{ random_seed } { }

    [[nodiscard]] MessageType type() const override;
    [[nodiscard]] decltype(MessageHeader::payload_size) payload_size() const override;
    [[nodiscard]] c2k::MessageBuffer serialize() const override;
    [[nodiscard]] static GameStart deserialize(c2k::MessageBuffer& buffer);

    [[nodiscard]] static constexpr decltype(MessageHeader::payload_size) max_payload_size() {
        return calculate_payload_size();
    }

private:
    [[nodiscard]] static constexpr decltype(MessageHeader::payload_size) calculate_payload_size() {
        return static_cast<decltype(MessageHeader::payload_size)>(
                sizeof(client_id) + sizeof(start_frame) + sizeof(random_seed)
        );
    }

    [[nodiscard]] bool equals(AbstractMessage const& other) const override {
        auto const& other_game_start = static_cast<decltype(*this)&>(other);
        return std::tie(client_id, start_frame, random_seed)
               == std::tie(other_game_start.client_id, other_game_start.start_frame, other_game_start.random_seed);
    }
};

struct EventBroadcast final : AbstractMessage {
    struct ClientEvents {
        std::uint8_t client_id;
        std::vector<Event> events;

        [[nodiscard]] bool operator==(ClientEvents const&) const = default;
    };

    std::uint64_t frame;
    std::vector<ClientEvents> events_per_client;

    EventBroadcast(std::uint64_t const frame, std::vector<ClientEvents> events_per_client)
        : frame{ frame },
          events_per_client{ std::move(events_per_client) } {
        assert(this->frame >= heartbeat_interval);
        if (this->events_per_client.size() > std::numeric_limits<std::uint8_t>::max()) {
            throw MessageInstantiationError{ fmt::format(
                    "cannot instantiate EventBroadcast message with {} clients ({} is maximum)",
                    this->events_per_client.size(),
                    std::numeric_limits<std::uint8_t>::max()
            ) };
        }

        auto contained_client_ids = std::unordered_set<decltype(ClientEvents::client_id)>{};

        for (auto const& [client_id, events] : this->events_per_client) {
            auto const [_, inserted] = contained_client_ids.insert(client_id);
            if (not inserted) {
                throw MessageInstantiationError{
                    std::format("duplicate client id {} while trying to instantiate EventBroadcast message", client_id)
                };
            }
            if (events.size() > heartbeat_interval or events.empty()) {
                throw MessageInstantiationError{
                    std::format("cannot instantiate EventBroadcast message with {} events for client", events.size())
                };
            }

            auto current_frame = events.at(0).frame;
            for (auto i = std::size_t{ 0 }; i < events.size(); ++i) {
                auto const next_frame = events.at(i).frame;
                if (i > 0) {
                    if (next_frame <= current_frame) {
                        throw MessageInstantiationError{ "frame numbers of events must be in ascending order" };
                    }
                    current_frame = next_frame;
                }
                if (next_frame > this->frame or next_frame < this->frame - heartbeat_interval + 1) {
                    throw MessageInstantiationError{ std::format(
                            "event at in frame {} is outside of valid bounds for message in frame {}",
                            next_frame,
                            this->frame
                    ) };
                }
            }
        }
    }

    [[nodiscard]] MessageType type() const override;
    [[nodiscard]] decltype(MessageHeader::payload_size) payload_size() const override;
    [[nodiscard]] c2k::MessageBuffer serialize() const override;
    [[nodiscard]] static EventBroadcast deserialize(c2k::MessageBuffer& buffer);

    [[nodiscard]] static constexpr decltype(MessageHeader::payload_size) max_payload_size() {
        auto const sizes = [] {
            auto values = std::array<std::size_t, std::numeric_limits<std::uint8_t>::max()>{};
            std::fill(values.begin(), values.end(), heartbeat_interval);
            return values;
        }();
        return calculate_payload_size(sizes);
    }

private:
    [[nodiscard]] static constexpr decltype(MessageHeader::payload_size) calculate_payload_size(
            std::span<std::size_t const> const sizes
    ) {
        auto result = std::size_t{ 0 };
        result += sizeof(frame);
        result += sizeof(std::uint8_t); // num clients
        for (auto const& size : sizes) {
            result += sizeof(std::uint8_t); // client id
            result += sizeof(std::uint8_t); // num events for this client
            result += size
                      * (sizeof(std::uint8_t)    // key
                         + sizeof(std::uint8_t)  // event type
                         + sizeof(std::uint64_t) // event frame
                      );
        }
        return static_cast<decltype(MessageHeader::payload_size)>(result);
    }

    [[nodiscard]] bool equals(AbstractMessage const& other) const override {
        auto const& other_event_broadcast = static_cast<decltype(*this)&>(other);
        return std::tie(frame, events_per_client)
               == std::tie(other_event_broadcast.frame, other_event_broadcast.events_per_client);
    }
};
