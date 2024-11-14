#pragma once

#include <spdlog/spdlog.h>
#include <array>
#include <memory>
#include <simulator/input.hpp>
#include <simulator/key_state.hpp>
#include <simulator/matrix.hpp>
#include <simulator/tetromino_type.hpp>
#include <sockets/sockets.hpp>
#include <vector>
#include "constants.hpp"
#include "message_header.hpp"
#include "message_types.hpp"

class EventDeserializationError final : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class MessageDeserializationError final : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class MessageInstantiationError final : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

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

static constexpr auto player_name_buffer_size = usize{ 32 };

struct Connect final : AbstractMessage {
    std::string player_name;

    explicit Connect(std::string_view player_name);

    [[nodiscard]] static constexpr decltype(MessageHeader::payload_size) max_payload_size() {
        return static_cast<decltype(MessageHeader::payload_size)>(player_name_buffer_size);
    }

    [[nodiscard]] MessageType type() const override;
    [[nodiscard]] decltype(MessageHeader::payload_size) payload_size() const override;
    [[nodiscard]] c2k::MessageBuffer serialize() const override;
    [[nodiscard]] static Connect deserialize(c2k::MessageBuffer& buffer);

    [[nodiscard]] bool equals(AbstractMessage const& other) const override;
};

struct Heartbeat final : AbstractMessage {
public:
    std::uint64_t frame;
    std::array<KeyState, heartbeat_interval> key_states;

private:
    static constexpr auto precalculated_payload_size =
        sizeof(frame) + std::tuple_size_v<decltype(key_states)> * sizeof(KeyState);

public:
    Heartbeat(std::uint64_t const frame, std::array<KeyState, heartbeat_interval> const key_states)
        : frame{ frame }, key_states{ key_states } {}

    [[nodiscard]] MessageType type() const override;

    [[nodiscard]] decltype(MessageHeader::payload_size) payload_size() const override {
        return precalculated_payload_size;
    }

    [[nodiscard]] c2k::MessageBuffer serialize() const override;
    [[nodiscard]] static Heartbeat deserialize(c2k::MessageBuffer& buffer);

    [[nodiscard]] static constexpr decltype(MessageHeader::payload_size) max_payload_size() {
        return precalculated_payload_size;
    }

private:
    [[nodiscard]] bool equals(AbstractMessage const& other) const override {
        auto const& other_heartbeat = static_cast<decltype(*this)&>(other);
        return std::tie(frame, key_states) == std::tie(other_heartbeat.frame, other_heartbeat.key_states);
    }
};

struct GridState final : AbstractMessage {
    std::uint64_t frame;
    std::array<TetrominoType, Matrix::width * Matrix::height> grid_contents;

    GridState(std::uint64_t const frame, std::array<TetrominoType, 10 * 22> const& grid_contents)
        : frame{ frame }, grid_contents{ grid_contents } {}

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

struct ClientIdentity final {
    u8 client_id;
    std::string player_name;

    ClientIdentity(u8 const client_id, std::string player_name)
        : client_id{ client_id }, player_name{ std::move(player_name) } {}

    [[nodiscard]] bool operator==(ClientIdentity const& other) const = default;
};

struct GameStart final : AbstractMessage {
    std::uint8_t client_id;
    std::uint64_t start_frame;
    std::uint64_t random_seed;
    std::vector<ClientIdentity> client_identities;

    GameStart(
        std::uint8_t const client_id,
        std::uint64_t const start_frame,
        std::uint64_t const random_seed,
        std::vector<ClientIdentity> client_identities
    )
        : client_id{ client_id },
          start_frame{ start_frame },
          random_seed{ random_seed },
          client_identities{ std::move(client_identities) } {
        if (this->client_identities.size() > std::numeric_limits<u8>::max()) {
            throw std::invalid_argument{ "Number of clients is too high." };
        }
    }

    [[nodiscard]] MessageType type() const override;
    [[nodiscard]] decltype(MessageHeader::payload_size) payload_size() const override;
    [[nodiscard]] c2k::MessageBuffer serialize() const override;
    [[nodiscard]] static GameStart deserialize(c2k::MessageBuffer& buffer);

    [[nodiscard]] static constexpr decltype(MessageHeader::payload_size) max_payload_size() {
        return calculate_payload_size(std::numeric_limits<u8>::max());
    }

    [[nodiscard]] u8 num_players() const {
        return gsl::narrow<u8>(client_identities.size());
    }

private:
    [[nodiscard]] static constexpr decltype(MessageHeader::payload_size) calculate_payload_size(u8 const num_players) {
        return static_cast<decltype(MessageHeader::payload_size)>(
            sizeof(client_id) + sizeof(start_frame) + sizeof(random_seed) + sizeof(u8) /* num players */
            + num_players * (sizeof(ClientIdentity::client_id) + player_name_buffer_size)
        );
    }

    [[nodiscard]] bool equals(AbstractMessage const& other) const override {
        auto const& other_game_start = static_cast<decltype(*this)&>(other);
        return std::tie(client_id, start_frame, random_seed, client_identities)
               == std::tie(
                   other_game_start.client_id,
                   other_game_start.start_frame,
                   other_game_start.random_seed,
                   other_game_start.client_identities
               );
    }
};

struct StateBroadcast final : AbstractMessage {
    struct ClientStates {
        std::uint8_t client_id = 0;
        std::array<KeyState, heartbeat_interval> states = {};

        [[nodiscard]] bool operator==(ClientStates const&) const = default;

        [[nodiscard]] static constexpr std::size_t size_in_bytes() {
            return sizeof(client_id) + std::tuple_size_v<decltype(states)> * sizeof(KeyState);
        }
    };

    std::uint64_t frame;
    std::vector<ClientStates> states_per_client;

    StateBroadcast(std::uint64_t frame, std::vector<ClientStates> states_per_client);
    [[nodiscard]] MessageType type() const override;
    [[nodiscard]] decltype(MessageHeader::payload_size) payload_size() const override;
    [[nodiscard]] c2k::MessageBuffer serialize() const override;
    [[nodiscard]] static StateBroadcast deserialize(c2k::MessageBuffer& buffer);

    [[nodiscard]] static constexpr decltype(MessageHeader::payload_size) max_payload_size() {
        return calculate_payload_size(std::numeric_limits<std::uint8_t>::max());
    }

private:
    // clang-format off
    [[nodiscard]] static constexpr decltype(MessageHeader::payload_size) calculate_payload_size(
        std::size_t const num_clients
    ) {  // clang-format on
        return gsl::narrow<decltype(MessageHeader::payload_size)>(
            sizeof(frame) + sizeof(std::uint8_t) + ClientStates::size_in_bytes() * num_clients
        );
    }

    [[nodiscard]] bool equals(AbstractMessage const& other) const override {
        auto const& other_event_broadcast = static_cast<decltype(*this)&>(other);
        return std::tie(frame, states_per_client)
               == std::tie(other_event_broadcast.frame, other_event_broadcast.states_per_client);
    }
};

struct ClientDisconnected final : AbstractMessage {
    u8 client_id;

    explicit ClientDisconnected(u8 const client_id)
        : client_id{ client_id } {}

    [[nodiscard]] MessageType type() const override;
    [[nodiscard]] decltype(MessageHeader::payload_size) payload_size() const override;
    [[nodiscard]] c2k::MessageBuffer serialize() const override;
    [[nodiscard]] static ClientDisconnected deserialize(c2k::MessageBuffer& buffer);

    [[nodiscard]] static constexpr decltype(MessageHeader::payload_size) max_payload_size() {
        return sizeof(client_id);
    }

private:
    [[nodiscard]] bool equals(AbstractMessage const& other) const override;
};
