#pragma once

#include "detail/shared_lib_export.hpp"
#include "message_types.hpp"
#include "message_header.hpp"
#include <array>
#include <memory>
#include <obpf/constants.h>
#include <obpf/input.h>
#include <obpf/tetromino_type.h>
#include <sockets/sockets.hpp>
#include <vector>

class MessageSerializationError final : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct Event final {
    ObpfKey key;
    ObpfEventType type;
    std::uint64_t frame;

    Event(ObpfKey const key, ObpfEventType const type, std::uint64_t const frame)
        : key{ key },
          type{ type },
          frame{ frame } { }

    [[nodiscard]] static Event deserialize(c2k::Extractor& buffer);

    [[nodiscard]] constexpr bool operator==(Event const&) const = default;
};

struct AbstractMessage {
    virtual ~AbstractMessage() = default;

    [[nodiscard]] bool operator==(AbstractMessage const& other) const {
        return typeid(*this) == typeid(other) and equals(other);
    }

    [[nodiscard]] EXPORT virtual MessageType type() const = 0;
    [[nodiscard]] EXPORT virtual decltype(MessageHeader::payload_size) payload_size() const = 0;
    [[nodiscard]] EXPORT virtual c2k::MessageBuffer serialize() const = 0;

    // clang-format off
    [[nodiscard]] EXPORT static std::unique_ptr<AbstractMessage> from_socket(
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

    [[nodiscard]] EXPORT MessageType type() const override;
    [[nodiscard]] EXPORT decltype(MessageHeader::payload_size) payload_size() const override;
    [[nodiscard]] EXPORT c2k::MessageBuffer serialize() const override;
    [[nodiscard]] EXPORT static Heartbeat deserialize(c2k::Extractor& buffer);

    [[nodiscard]] static constexpr decltype(MessageHeader::payload_size) max_payload_size() {
        return calculate_payload_size(heartbeat_interval);
    }

private:
    [[nodiscard]] static constexpr std::uint64_t calculate_payload_size(std::size_t const num_events) {
        // clang-format off
        return
            sizeof(frame)
            + sizeof(std::uint8_t) // number of events
            + num_events * (
                sizeof(std::uint8_t) // key
                + sizeof(std::uint8_t) // type
                + sizeof(std::uint64_t) // frame
            );
        // clang-format on
    }

    [[nodiscard]] bool equals(AbstractMessage const& other) const override {
        auto const& other_heartbeat = static_cast<decltype(*this) const&>(other);
        return std::tie(frame, events) == std::tie(other_heartbeat.frame, other_heartbeat.events);
    }
};

struct GridState final : AbstractMessage {
    std::uint64_t frame;
    std::array<ObpfTetrominoType, OBPF_MATRIX_WIDTH * OBPF_MATRIX_HEIGHT> grid_contents;

    GridState(std::uint64_t const frame, std::array<ObpfTetrominoType, 10 * 22> const& grid_contents)
        : frame{ frame },
          grid_contents{ grid_contents } { }

    [[nodiscard]] EXPORT MessageType type() const override;
    [[nodiscard]] EXPORT decltype(MessageHeader::payload_size) payload_size() const override;
    [[nodiscard]] EXPORT c2k::MessageBuffer serialize() const override;
    [[nodiscard]] EXPORT static GridState deserialize(c2k::Extractor& buffer);

    [[nodiscard]] static constexpr decltype(MessageHeader::payload_size) max_payload_size() {
        return calculate_payload_size();
    }

private:
    [[nodiscard]] static constexpr std::uint64_t calculate_payload_size() {
        return sizeof(frame) + std::tuple_size_v<decltype(grid_contents)> * sizeof(std::uint8_t);
    }

    [[nodiscard]] bool equals(AbstractMessage const& other) const override {
        auto const& other_gridstate = static_cast<decltype(*this) const&>(other);
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

    [[nodiscard]] EXPORT MessageType type() const override;
    [[nodiscard]] EXPORT decltype(MessageHeader::payload_size) payload_size() const override;
    [[nodiscard]] EXPORT c2k::MessageBuffer serialize() const override;
    [[nodiscard]] EXPORT static GameStart deserialize(c2k::Extractor& buffer);

    [[nodiscard]] static constexpr decltype(MessageHeader::payload_size) max_payload_size() {
        return calculate_payload_size();
    }

private:
    [[nodiscard]] static constexpr std::uint64_t calculate_payload_size() {
        return sizeof(client_id) + sizeof(start_frame) + sizeof(random_seed);
    }

    [[nodiscard]] bool equals(AbstractMessage const& other) const override {
        auto const& other_game_start = static_cast<decltype(*this) const&>(other);
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
          events_per_client{ std::move(events_per_client) } { }

    [[nodiscard]] EXPORT MessageType type() const override;
    [[nodiscard]] EXPORT decltype(MessageHeader::payload_size) payload_size() const override;
    [[nodiscard]] EXPORT c2k::MessageBuffer serialize() const override;
    [[nodiscard]] EXPORT static EventBroadcast deserialize(c2k::Extractor& buffer);

    [[nodiscard]] static constexpr decltype(MessageHeader::payload_size) max_payload_size() {
        auto const sizes = [] {
            auto values = std::array<std::size_t, std::numeric_limits<std::uint8_t>::max()>{};
            std::fill(values.begin(), values.end(), heartbeat_interval);
            return values;
        }();
        return calculate_payload_size(sizes);
    }

private:
    [[nodiscard]] static constexpr std::uint64_t calculate_payload_size(std::span<std::size_t const> const sizes) {
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
        return static_cast<std::uint64_t>(result);
    }

    [[nodiscard]] bool equals(AbstractMessage const& other) const override {
        auto const& other_event_broadcast = static_cast<decltype(*this) const&>(other);
        return std::tie(frame, events_per_client)
               == std::tie(other_event_broadcast.frame, other_event_broadcast.events_per_client);
    }
};
