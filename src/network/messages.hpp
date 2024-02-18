#pragma once

#include "message_types.hpp"
#include "obpf/constants.h"

#include <array>
#include <cassert>
#include <limits>
#include <memory>
#include <obpf/input.h>
#include <obpf/tetromino_type.h>
#include <sockets/sockets.hpp>
#include <vector>

struct Event final {
    ObpfKey key;
    ObpfEventType type;
    std::uint64_t frame;

    Event(ObpfKey const key, ObpfEventType const type, std::uint64_t const frame)
        : key{ key },
          type{ type },
          frame{ frame } { }

    [[nodiscard]] static Event deserialize(c2k::Extractor& buffer) {
        auto const key = static_cast<ObpfKey>(buffer.try_extract<std::uint8_t>().value());
        auto const event_type = static_cast<ObpfEventType>(buffer.try_extract<std::uint8_t>().value());
        auto const event_frame = buffer.try_extract<std::uint64_t>().value();
        return Event{ key, event_type, event_frame };
    }
};

inline c2k::MessageBuffer& operator<<(c2k::MessageBuffer& buffer, Event const& event) {
    return buffer << static_cast<std::uint8_t>(event.key) << static_cast<std::uint8_t>(event.type) << event.frame;
}

struct AbstractMessage {
    virtual ~AbstractMessage() = default;

    virtual MessageType type() const = 0;
    virtual std::uint64_t payload_size() const = 0;
    virtual c2k::MessageBuffer serialize() const = 0;

    [[nodiscard]] static std::unique_ptr<AbstractMessage> from_socket(c2k::ClientSocket& socket);
};

struct Heartbeat final : AbstractMessage {
    std::uint64_t frame;
    std::vector<Event> events; // at least 0, at most 15

    [[nodiscard]] MessageType type() const override {
        return MessageType::Heartbeat;
    }

    [[nodiscard]] std::uint64_t payload_size() const override {
        // clang-format off
        return
            sizeof(frame)
            + sizeof(std::uint8_t) // number of events
            + events.size() * (
                sizeof(std::uint8_t) // key
                + sizeof(std::uint8_t) // type
                + sizeof(std::uint64_t) // frame
            );
        // clang-format on
    }

    [[nodiscard]] c2k::MessageBuffer serialize() const override {
        assert(events.size() <= 15);
        auto buffer = c2k::MessageBuffer{};
        // clang-format off
        buffer << static_cast<std::uint8_t>(MessageType::Heartbeat)
               << payload_size()
               << frame
               << static_cast<std::uint8_t>(events.size());
        // clang-format on
        for (auto const& event : events) {
            buffer << event;
        }
        assert(buffer.data.size() == payload_size());
        return buffer;
    }

    [[nodiscard]] static Heartbeat deserialize(c2k::Extractor& buffer) {
        auto result = Heartbeat{};
        result.frame = buffer.try_extract<std::uint64_t>().value();
        auto const num_events = static_cast<std::size_t>(buffer.try_extract<std::uint8_t>().value());
        auto events = std::vector<Event>{};
        events.reserve(num_events);
        for (auto i = std::size_t{ 0 }; i < num_events; ++i) {
            events.push_back(Event::deserialize(buffer));
        }
        assert(buffer.size() == 0);
        result.events = std::move(events);
        return result;
    }
};

struct GridState final : AbstractMessage {
    std::uint64_t frame;
    std::array<ObpfTetrominoType, OBPF_MATRIX_WIDTH * OBPF_MATRIX_HEIGHT> grid_contents;

    [[nodiscard]] MessageType type() const override {
        return MessageType::GridState;
    }

    [[nodiscard]] std::uint64_t payload_size() const override {
        return sizeof(frame) + grid_contents.size() * sizeof(std::uint8_t);
    }

    [[nodiscard]] c2k::MessageBuffer serialize() const override {
        auto buffer = c2k::MessageBuffer{};
        // clang-format off
        buffer << static_cast<std::uint8_t>(MessageType::GridState)
               << payload_size()
               << frame;
        // clang-format on
        for (auto const tetromino_type : grid_contents) {
            buffer << static_cast<std::uint8_t>(tetromino_type);
        }
        assert(buffer.data.size() == payload_size());
        return buffer;
    }

    [[nodiscard]] static GridState deserialize(c2k::Extractor& buffer) {
        auto result = GridState{};
        result.frame = buffer.try_extract<std::uint64_t>().value();
        for (auto& tetromino : result.grid_contents) {
            tetromino = static_cast<ObpfTetrominoType>(buffer.try_extract<std::uint8_t>().value());
        }
        assert(buffer.size() == 0);
        return result;
    }
};

struct GameStart final : AbstractMessage {
    std::uint8_t client_id;
    std::uint64_t start_frame;
    std::uint64_t random_seed;

    [[nodiscard]] MessageType type() const override {
        return MessageType::GameStart;
    }

    [[nodiscard]] std::uint64_t payload_size() const override {
        return sizeof(client_id) + sizeof(start_frame) + sizeof(random_seed);
    }

    [[nodiscard]] c2k::MessageBuffer serialize() const override {
        auto buffer = c2k::MessageBuffer{};
        // clang-format off
        buffer << static_cast<std::uint8_t>(MessageType::GameStart)
               << payload_size()
               << client_id
               << start_frame
               << random_seed;
        // clang-format on
        assert(buffer.data.size() == payload_size());
        return buffer;
    }

    [[nodiscard]] static GameStart deserialize(c2k::Extractor& buffer) {
        auto result = GameStart{};
        std::tie(result.client_id, result.start_frame, result.random_seed) =
                buffer.try_extract<std::uint8_t, std::uint64_t, std::uint64_t>().value();
        assert(buffer.size() == 0);
        return result;
    }
};

struct EventBroadcast final : AbstractMessage {
    struct ClientEvents {
        std::uint8_t client_id;
        std::vector<Event> events;
    };

    std::uint64_t frame;
    std::vector<ClientEvents> events_per_client;

    [[nodiscard]] MessageType type() const override {
        return MessageType::EventBroadcast;
    }

    [[nodiscard]] std::uint64_t payload_size() const override {
        auto result = std::size_t{ 0 };
        result += sizeof(frame);
        result += sizeof(std::uint8_t); // num clients
        for (auto const& events : events_per_client) {
            result += sizeof(std::uint8_t); // client id
            result += sizeof(std::uint8_t); // num events for this client
            result += events.events.size()
                      * (sizeof(std::uint8_t)    // key
                         + sizeof(std::uint8_t)  // event type
                         + sizeof(std::uint64_t) // event frame
                      );
        }
        return static_cast<std::uint64_t>(result);
    }

    [[nodiscard]] c2k::MessageBuffer serialize() const override {
        assert(events_per_client.size() <= std::numeric_limits<std::uint8_t>::max());
        auto buffer = c2k::MessageBuffer{};
        // clang-format off
        buffer << static_cast<std::uint8_t>(MessageType::EventBroadcast)
               << payload_size()
               << frame
               << static_cast<std::uint8_t>(events_per_client.size()); // num clients
        // clang-format on
        for (auto const& events : events_per_client) {
            assert(events.events.size() <= 15);
            buffer << events.client_id << static_cast<std::uint8_t>(events.events.size());
            for (auto const& event : events.events) {
                buffer << event;
            }
        }
        assert(buffer.data.size() == payload_size());
        return buffer;
    }

    [[nodiscard]] static EventBroadcast deserialize(c2k::Extractor& buffer) {
        auto result = EventBroadcast{};
        auto const num_clients = static_cast<std::size_t>(buffer.try_extract<std::uint8_t>().value());
        result.events_per_client.reserve(num_clients);
        for (auto i = std::size_t{ 0 }; i < num_clients; ++i) {
            auto events = ClientEvents{};
            events.client_id = buffer.try_extract<std::uint8_t>().value();
            auto const num_events = static_cast<std::size_t>(buffer.try_extract<std::uint8_t>().value());
            events.events.reserve(num_events);
            for (auto j = std::size_t{ 0 }; j < num_events; ++j) {
                events.events.push_back(Event::deserialize(buffer));
            }
            result.events_per_client.push_back(std::move(events));
        }
        assert(buffer.size() == 0);
        return result;
    }
};
