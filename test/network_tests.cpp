#include "utils.hpp"
#include <gmock/gmock-matchers.h>
#include <gsl/gsl>
#include <gtest/gtest.h>
#include <network/constants.hpp>
#include <network/messages.hpp>
#include <random>
#include <spdlog/spdlog.h>

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
          Event{ Key::Left, EventType::Pressed, 27 },
          Event{ Key::Drop, EventType::Pressed, 32 },
          Event{ Key::Left, EventType::Released, 35 },
          Event{ Key::Drop, EventType::Released, 42 },
          },
    };
    auto const deserialized_message = send_receive_and_deserialize(message);

    auto const deserialized_heartbeat = dynamic_cast<Heartbeat const*>(deserialized_message.get());
    ASSERT_NE(deserialized_heartbeat, nullptr);
    EXPECT_EQ(*deserialized_heartbeat, message);
}

TEST(NetworkTests, BiggestAllowedHeartbeatMessage) {
    static constexpr auto event_pattern = std::array{
        std::tuple{  Key::Left,  EventType::Pressed },
        std::tuple{ Key::Right,  EventType::Pressed },
        std::tuple{  Key::Drop,  EventType::Pressed },
        std::tuple{  Key::Left, EventType::Released },
        std::tuple{ Key::Right, EventType::Released },
        std::tuple{  Key::Drop, EventType::Released },
    };
    auto events = std::vector<Event>{};
    // clang-format off
    for (
        auto i = std::size_t{ 0 }, pattern_index = std::size_t{ 0 };
        i < heartbeat_interval;
        ++i, pattern_index = (pattern_index + 1) % event_pattern.size()
    ) { // clang-format on
        auto const [key, type] = event_pattern[pattern_index];
        events.emplace_back(key, type, 42 + i);
    }
    auto const message = Heartbeat{ 42 + events.size(), std::move(events) };

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
        EXPECT_STREQ(e.what(), "message payload size 160 is too big for message type 0 (maximum is 159)");
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
    buffer << std::to_underlying(MessageType::GridState) << std::uint16_t{ 227 }; // 1 byte too few
    for (auto i = 0; i < 300; ++i) {
        buffer << std::uint8_t{ 42 };
    }
    EXPECT_THROW({ std::ignore = send_receive_buffer_and_deserialize(buffer); }, MessageDeserializationError);
}

TEST(NetworkTests, SlightlyTooBigGridStateMessageFails) {
    auto buffer = c2k::MessageBuffer{};
    buffer << std::to_underlying(MessageType::GridState) << std::uint16_t{ 229 }; // 1 byte too many
    for (auto i = 0; i < 300; ++i) {
        buffer << std::uint8_t{ 42 };
    }
    EXPECT_THROW({ std::ignore = send_receive_buffer_and_deserialize(buffer); }, MessageDeserializationError);
}

TEST(NetworkTests, GameStartMessage) {
    auto const random_seed = static_cast<std::uint64_t>(std::random_device{}());
    auto const message = GameStart{ 31, 180, random_seed };

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
    buffer << std::to_underlying(MessageType::GameStart) << std::uint16_t{ 16 }; // 1 byte too few
    for (auto i = 0; i < 300; ++i) {
        buffer << std::uint8_t{ 42 };
    }
    EXPECT_THROW({ std::ignore = send_receive_buffer_and_deserialize(buffer); }, MessageDeserializationError);
}

TEST(NetworkTests, SlightlyTooBigGameStartMessageFails) {
    auto buffer = c2k::MessageBuffer{};
    buffer << std::to_underlying(MessageType::GameStart) << std::uint16_t{ 18 }; // 1 byte too many
    for (auto i = 0; i < 300; ++i) {
        buffer << std::uint8_t{ 42 };
    }
    EXPECT_THROW({ std::ignore = send_receive_buffer_and_deserialize(buffer); }, MessageDeserializationError);
}

TEST(NetworkTests, MinimalEventBroadcastMessage) {
    auto const message = EventBroadcast{ 42, {} };

    auto const deserialized_message = send_receive_and_deserialize(message);
    EXPECT_EQ(dynamic_cast<EventBroadcast const&>(*deserialized_message), message);
}

TEST(NetworkTests, MaximumEventBroadcastMessage) {
    auto events_per_client = std::vector<EventBroadcast::ClientEvents>{};
    for (auto i = 0; i < std::numeric_limits<std::uint8_t>::max(); ++i) {
        auto events = std::vector<Event>{};
        for (auto j = std::size_t{ 0 }; j < heartbeat_interval; ++j) {
            events.emplace_back(Key::Left, EventType::Pressed, 42 - heartbeat_interval + 1 + j);
        }
        events_per_client.emplace_back(static_cast<std::uint8_t>(i), std::move(events));
    }
    auto const message = EventBroadcast{ 42, std::move(events_per_client) };

    auto const deserialized_message = send_receive_and_deserialize(message);
    EXPECT_EQ(dynamic_cast<EventBroadcast const&>(*deserialized_message), message);
}

TEST(NetworkTests, SlightlyTooBigEventBroadcastMessage) {
    auto buffer = c2k::MessageBuffer{};
    static constexpr auto event_max_payload_size =
            sizeof(EventBroadcast::frame) + sizeof(std::uint8_t)
            + std::numeric_limits<std::uint8_t>::max() * 2 * sizeof(std::uint8_t)
            + std::numeric_limits<std::uint8_t>::max() * heartbeat_interval
                      * (sizeof(std::uint8_t) + sizeof(std::uint8_t) + sizeof(Event::frame));
    buffer << std::to_underlying(MessageType::EventBroadcast) << gsl::narrow<std::uint16_t>(event_max_payload_size + 1);

    for (auto i = std::size_t{ 0 }; i < event_max_payload_size + 1; ++i) {
        buffer << std::uint8_t{ 42 };
    }

    EXPECT_THROW({ std::ignore = send_receive_buffer_and_deserialize(buffer); }, MessageDeserializationError);
}

TEST(NetworkTests, EventBroadcastMessageWithEmptyClientFails) {
    auto buffer = c2k::MessageBuffer{};
    // clang-format off
    buffer << std::to_underlying(MessageType::EventBroadcast)
           << std::uint16_t{ 11 } // payload size
           << std::uint64_t{ 42 } // frame
           << std::uint8_t{ 1 }   // client_count
           << std::uint8_t{ 99 }  // client_id
           << std::uint8_t{ 0 };  // event count (invalid value)
    // clang-format on
    EXPECT_THROW({ std::ignore = send_receive_buffer_and_deserialize(buffer); }, MessageDeserializationError);
}

TEST(NetworkTests, EventBroadcastMessageWithTooManyEventsFails) {
    auto buffer = c2k::MessageBuffer{};
    // clang-format off
    buffer << std::to_underlying(MessageType::EventBroadcast)
           << std::uint16_t{ 11 }  // payload size
           << std::uint64_t{ 42 }  // frame
           << std::uint8_t{ 1 }    // client_count
           << std::uint8_t{ 99 }   // client_id
           << std::uint8_t{ 255 }; // event count (invalid value)
    // clang-format on
    EXPECT_THROW({ std::ignore = send_receive_buffer_and_deserialize(buffer); }, MessageDeserializationError);
}

TEST(NetworkTests, EventBroadcastMessageWithOneTooManyEventsFails) {
    auto buffer = c2k::MessageBuffer{};
    // clang-format off
    buffer << std::to_underlying(MessageType::EventBroadcast)
           << std::uint16_t{ 11 }  // payload size
           << std::uint64_t{ 42 }  // frame
           << std::uint8_t{ 1 }    // client_count
           << std::uint8_t{ 99 }   // client_id
           << std::uint8_t{ heartbeat_interval + 1 }; // event count (invalid value)
    // clang-format on
    EXPECT_THROW({ std::ignore = send_receive_buffer_and_deserialize(buffer); }, MessageDeserializationError);
}

TEST(NetworkTests, EventBroadcastMessageWithDuplicateClientIdsFails) {
    auto buffer = c2k::MessageBuffer{};
    // clang-format off
    buffer << std::to_underlying(MessageType::EventBroadcast)
           << std::uint16_t{ 33 }  // payload size
           << std::uint64_t{ 42 }  // frame
           << std::uint8_t{ 2 }    // client_count
           << std::uint8_t{ 15 }   // client_id
           << std::uint8_t{ 1 }    // event count
           << gsl::narrow<std::uint8_t>(Key::Drop)
           << gsl::narrow<std::uint8_t>(EventType::Pressed)
           << std::uint64_t{ 35 }
           << std::uint8_t{ 15 }   // client_id (duplicate)
           << std::uint8_t{ 1 }    // event count
           << gsl::narrow<std::uint8_t>(Key::Drop)
           << gsl::narrow<std::uint8_t>(EventType::Pressed)
           << std::uint64_t{ 35 };
    // clang-format on
    EXPECT_THROW({ std::ignore = send_receive_buffer_and_deserialize(buffer); }, MessageDeserializationError);
}
