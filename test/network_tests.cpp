#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <gsl/gsl>
#include <network/constants.hpp>
#include <network/messages.hpp>
#include <random>
#include "utils.hpp"

TEST(NetworkTests, UnknownMessageTypeFails) {
    auto buffer = c2k::MessageBuffer{};
    // clang-format off
    buffer << std::uint8_t{ 17 }   // unknown message type
           << std::uint16_t{ 10 }; // payload size
    // clang-format on
    EXPECT_THROW({ std::ignore = send_receive_buffer_and_deserialize(buffer); }, MessageDeserializationError);
}

TEST(NetworkTests, MinimalHeartbeatMessage) {
    auto const message = Heartbeat{ 42, {} };
    auto const deserialized_message = send_receive_and_deserialize(message);

    auto const deserialized_heartbeat = dynamic_cast<Heartbeat const*>(deserialized_message.get());
    ASSERT_NE(deserialized_heartbeat, nullptr);
    EXPECT_EQ(*deserialized_heartbeat, message);
}

TEST(NetworkTests, RegularHeartbeatMessage) {
    auto const message = Heartbeat{
        42,
        {
          KeyState{}.set(Key::Left),
          KeyState{},
          KeyState{},
          KeyState{}.set(Key::Right),
          KeyState{},
          KeyState{},
          KeyState{},
          KeyState{}.set(Key::Left, false),
          KeyState{},
          KeyState{},
          KeyState{},
          KeyState{}.set(Key::Right, false),
          KeyState{},
          KeyState{},
          KeyState{},
          },
    };
    auto const deserialized_message = send_receive_and_deserialize(message);

    auto const deserialized_heartbeat = dynamic_cast<Heartbeat const*>(deserialized_message.get());
    ASSERT_NE(deserialized_heartbeat, nullptr);
    EXPECT_EQ(*deserialized_heartbeat, message);
}

TEST(NetworkTests, TooBigHeartbeatMessageFails) {
    auto buffer = c2k::MessageBuffer{};
    buffer << static_cast<std::uint8_t>(MessageType::Heartbeat)
           << static_cast<decltype(MessageHeader::payload_size)>(Heartbeat::max_payload_size() + 1);

    try {
        std::ignore = send_receive_buffer_and_deserialize(buffer);
        FAIL() << "expected MessageSerializationError";
    } catch (MessageDeserializationError const& e) {
        EXPECT_STREQ(e.what(), "message payload size 24 is too big for message type 1 (maximum is 23)");
    } catch (...) {
        FAIL() << "expected MessageSerializationError";
    }
}

TEST(NetworkTests, HeartbeatMessageWithEmptyPayloadFails) {
    auto buffer = c2k::MessageBuffer{};
    buffer << static_cast<std::uint8_t>(MessageType::Heartbeat) << std::uint16_t{ 0 };

    try {
        std::ignore = send_receive_buffer_and_deserialize(buffer);
        FAIL() << "expected MessageSerializationError";
    } catch (MessageDeserializationError const& e) {
        EXPECT_STREQ(e.what(), "message payload size 0 is invalid");
    } catch (...) {
        FAIL() << "expected MessageSerializationError";
    }
}

TEST(NetworkTests, HeartbeatMessageWithLessDataThanDeclaredInHeader) {
    auto buffer = c2k::MessageBuffer{};
    buffer << static_cast<std::uint8_t>(MessageType::Heartbeat) << std::uint16_t{ 10 } << std::uint8_t{ 1 }
           << std::uint8_t{ 2 } << std::uint8_t{ 3 };

    EXPECT_THROW({ std::ignore = send_receive_buffer_and_deserialize(buffer); }, c2k::TimeoutError);
}

TEST(NetworkTests, GridStateMessage) {
    static constexpr auto grid_contents = [] {
        auto result = std::array<TetrominoType, Matrix::width * Matrix::height>{};
        auto current = static_cast<TetrominoType>(0);
        for (auto& mino : result) {
            mino = current;
            current = static_cast<TetrominoType>(
                (std::to_underlying(current) + 1) % (std::to_underlying(TetrominoType::Last) + 1)
            );
        }
        return result;
    }();
    auto const message = GridState{ 42, grid_contents };

    auto const deserialized_message = send_receive_and_deserialize(message);

    auto const deserialized_grid_state = dynamic_cast<GridState const*>(deserialized_message.get());
    ASSERT_NE(deserialized_grid_state, nullptr);
    EXPECT_EQ(*deserialized_grid_state, message);
}

TEST(NetworkTests, EmptyGridStateMessageFails) {
    auto buffer = c2k::MessageBuffer{};
    buffer << std::to_underlying(MessageType::GridState) << std::uint16_t{ 0 };
    for (auto i = 0; i < 300; ++i) {
        buffer << std::uint8_t{ 42 };
    }
    EXPECT_THROW({ std::ignore = send_receive_buffer_and_deserialize(buffer); }, MessageDeserializationError);
}

TEST(NetworkTests, SlightlyTooSmallGridStateMessageFails) {
    auto buffer = c2k::MessageBuffer{};
    buffer << std::to_underlying(MessageType::GridState) << std::uint16_t{ 227 };  // 1 byte too few
    for (auto i = 0; i < 300; ++i) {
        buffer << std::uint8_t{ 42 };
    }
    EXPECT_THROW({ std::ignore = send_receive_buffer_and_deserialize(buffer); }, MessageDeserializationError);
}

TEST(NetworkTests, SlightlyTooBigGridStateMessageFails) {
    auto buffer = c2k::MessageBuffer{};
    buffer << std::to_underlying(MessageType::GridState) << std::uint16_t{ 229 };  // 1 byte too many
    for (auto i = 0; i < 300; ++i) {
        buffer << std::uint8_t{ 42 };
    }
    EXPECT_THROW({ std::ignore = send_receive_buffer_and_deserialize(buffer); }, MessageDeserializationError);
}

TEST(NetworkTests, GameStartMessage) {
    auto const random_seed = static_cast<std::uint64_t>(std::random_device{}());
    auto const message = GameStart{
        31,
        180,
        random_seed,
        std::vector{
                    ClientIdentity{ 0, "player0" },
                    ClientIdentity{ 1, "player1" },
                    ClientIdentity{ 2, "player2" },
                    },
    };

    auto const deserialized_message = send_receive_and_deserialize(message);

    auto const deserialized_game_start = dynamic_cast<GameStart const*>(deserialized_message.get());
    ASSERT_NE(deserialized_game_start, nullptr);
    EXPECT_EQ(*deserialized_game_start, message);
}

TEST(NetworkTests, EmptyGameStartMessageFails) {
    auto buffer = c2k::MessageBuffer{};
    buffer << std::to_underlying(MessageType::GameStart) << std::uint16_t{ 0 };
    for (auto i = 0; i < 300; ++i) {
        buffer << std::uint8_t{ 42 };
    }
    EXPECT_THROW({ std::ignore = send_receive_buffer_and_deserialize(buffer); }, MessageDeserializationError);
}

TEST(NetworkTests, SlightlyTooSmallGameStartMessageFails) {
    auto buffer = c2k::MessageBuffer{};
    buffer << std::to_underlying(MessageType::GameStart) << std::uint16_t{ 16 };  // 1 byte too few
    for (auto i = 0; i < 300; ++i) {
        buffer << std::uint8_t{ 42 };
    }
    EXPECT_THROW({ std::ignore = send_receive_buffer_and_deserialize(buffer); }, MessageDeserializationError);
}

TEST(NetworkTests, SlightlyTooBigGameStartMessageFails) {
    auto buffer = c2k::MessageBuffer{};
    buffer << std::to_underlying(MessageType::GameStart) << std::uint16_t{ 19 };  // 1 byte too many
    for (auto i = 0; i < 300; ++i) {
        buffer << std::uint8_t{ 42 };
    }
    EXPECT_THROW({ std::ignore = send_receive_buffer_and_deserialize(buffer); }, MessageDeserializationError);
}

TEST(NetworkTests, MinimalStateBroadcastMessage) {
    auto const message = StateBroadcast{ 14, {} };

    auto const deserialized_message = send_receive_and_deserialize(message);
    EXPECT_EQ(dynamic_cast<StateBroadcast const&>(*deserialized_message), message);
}

TEST(NetworkTests, MaximumStateBroadcastMessage) {
    auto events_per_client = std::vector<StateBroadcast::ClientStates>{};
    for (auto i = 0; i < std::numeric_limits<std::uint8_t>::max(); ++i) {
        events_per_client.emplace_back(
            gsl::narrow<u8>(i),
            std::array{
                KeyState{}.set(Key::Left),
                KeyState{},
                KeyState{},
                KeyState{}.set(Key::Right),
                KeyState{},
                KeyState{},
                KeyState{},
                KeyState{}.set(Key::Left, false),
                KeyState{},
                KeyState{},
                KeyState{},
                KeyState{}.set(Key::Right, false),
                KeyState{},
                KeyState{},
                KeyState{},
            }
        );
    }
    auto const message = StateBroadcast{ 14, std::move(events_per_client) };

    auto const deserialized_message = send_receive_and_deserialize(message);
    EXPECT_EQ(dynamic_cast<StateBroadcast const&>(*deserialized_message), message);
}

TEST(NetworkTests, SlightlyTooBigStateBroadcastMessage) {
    auto buffer = c2k::MessageBuffer{};
    buffer << std::to_underlying(MessageType::StateBroadcast)
           << gsl::narrow<std::uint16_t>(StateBroadcast::max_payload_size() + 1);

    for (auto i = std::size_t{ 0 }; i < StateBroadcast::max_payload_size() + 1; ++i) {
        buffer << std::uint8_t{ 14 };
    }

    EXPECT_THROW({ std::ignore = send_receive_buffer_and_deserialize(buffer); }, MessageDeserializationError);
}

TEST(NetworkTests, EventBroadcastMessageWithDuplicateClientIdsFails) {
    auto buffer = c2k::MessageBuffer{};
    // clang-format off
    buffer << std::to_underlying(MessageType::StateBroadcast)
           << std::uint16_t{ 41 }  // payload size
           << std::uint64_t{ 14 }  // frame
           << std::uint8_t{ 2 }    // client_count
           << std::uint8_t{ 15 }   // client_id
           << std::uint8_t{ 0 }    // key_state[0]
           << std::uint8_t{ 0 }    // key_state[1]
           << std::uint8_t{ 0 }    // key_state[2]
           << std::uint8_t{ 0 }    // key_state[3]
           << std::uint8_t{ 0 }    // key_state[4]
           << std::uint8_t{ 0 }    // key_state[5]
           << std::uint8_t{ 0 }    // key_state[6]
           << std::uint8_t{ 0 }    // key_state[7]
           << std::uint8_t{ 0 }    // key_state[8]
           << std::uint8_t{ 0 }    // key_state[9]
           << std::uint8_t{ 0 }    // key_state[10]
           << std::uint8_t{ 0 }    // key_state[11]
           << std::uint8_t{ 0 }    // key_state[12]
           << std::uint8_t{ 0 }    // key_state[13]
           << std::uint8_t{ 0 }    // key_state[14]
           << std::uint8_t{ 15 }   // client_id (duplicate)
           << std::uint8_t{ 0 }    // key_state[0]
           << std::uint8_t{ 0 }    // key_state[1]
           << std::uint8_t{ 0 }    // key_state[2]
           << std::uint8_t{ 0 }    // key_state[3]
           << std::uint8_t{ 0 }    // key_state[4]
           << std::uint8_t{ 0 }    // key_state[5]
           << std::uint8_t{ 0 }    // key_state[6]
           << std::uint8_t{ 0 }    // key_state[7]
           << std::uint8_t{ 0 }    // key_state[8]
           << std::uint8_t{ 0 }    // key_state[9]
           << std::uint8_t{ 0 }    // key_state[10]
           << std::uint8_t{ 0 }    // key_state[11]
           << std::uint8_t{ 0 }    // key_state[12]
           << std::uint8_t{ 0 }    // key_state[13]
           << std::uint8_t{ 0 };    // key_state[14]
    // clang-format on
    EXPECT_THROW({ std::ignore = send_receive_buffer_and_deserialize(buffer); }, MessageDeserializationError);
}
