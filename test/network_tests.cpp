#include <future>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include <network/constants.hpp>
#include <network/messages.hpp>

[[nodiscard]] static std::unique_ptr<AbstractMessage> send_receive_buffer_and_deserialize(
        c2k::MessageBuffer const& buffer
) {
    using namespace std::chrono_literals;
    auto promise = std::promise<std::unique_ptr<AbstractMessage>>{};
    auto future = promise.get_future();

    auto server = c2k::Sockets::create_server(c2k::AddressFamily::Ipv4, 0, [&promise](c2k::ClientSocket client) {
        try {
            auto message = AbstractMessage::from_socket(client);
            promise.set_value(std::move(message));
        } catch (...) {
            try {
                promise.set_exception(std::current_exception());
            } catch (...) {
                std::cerr << "something went horribly wrong!\n";
                std::terminate();
            }
        }
    });
    auto const port = server.local_address().port;
    auto client = c2k::Sockets::create_client(c2k::AddressFamily::Ipv4, "127.0.0.1", port);
    [[maybe_unused]] auto const num_bytes_sent = client.send(buffer).get();
    assert(num_bytes_sent == buffer.data.size());
    auto result = future.get();
    return result;
}

[[nodiscard]] static std::unique_ptr<AbstractMessage> send_receive_and_deserialize(AbstractMessage const& message) {
    auto const buffer = message.serialize();
    return send_receive_buffer_and_deserialize(buffer);
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
          Event{ ObpfKey::OBPF_KEY_LEFT, ObpfEventType::OBPF_PRESSED, 27 },
          Event{ ObpfKey::OBPF_KEY_DROP, ObpfEventType::OBPF_PRESSED, 32 },
          Event{ ObpfKey::OBPF_KEY_LEFT, ObpfEventType::OBPF_RELEASED, 35 },
          Event{ ObpfKey::OBPF_KEY_DROP, ObpfEventType::OBPF_RELEASED, 42 },
          },
    };
    auto const deserialized_message = send_receive_and_deserialize(message);

    auto const deserialized_heartbeat = dynamic_cast<Heartbeat const*>(deserialized_message.get());
    ASSERT_NE(deserialized_heartbeat, nullptr);
    EXPECT_EQ(*deserialized_heartbeat, message);
}

TEST(NetworkTests, BiggestAllowedHeartbeatMessage) {
    static constexpr auto event_pattern = std::array{
        std::tuple{  ObpfKey::OBPF_KEY_LEFT,  ObpfEventType::OBPF_PRESSED },
        std::tuple{ ObpfKey::OBPF_KEY_RIGHT,  ObpfEventType::OBPF_PRESSED },
        std::tuple{  ObpfKey::OBPF_KEY_DROP,  ObpfEventType::OBPF_PRESSED },
        std::tuple{  ObpfKey::OBPF_KEY_LEFT, ObpfEventType::OBPF_RELEASED },
        std::tuple{ ObpfKey::OBPF_KEY_RIGHT, ObpfEventType::OBPF_RELEASED },
        std::tuple{  ObpfKey::OBPF_KEY_DROP, ObpfEventType::OBPF_RELEASED },
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
    } catch (const MessageSerializationError& e) {
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
    } catch (const MessageSerializationError& e) {
        EXPECT_STREQ(e.what(), "message payload size 0 is invalid");
    } catch (...) {
        FAIL() << "expected MessageSerializationError";
    }
}

TEST(NetworkTests, HeartbeatMessageWithLessDataThanDeclaredInHeader) {
    auto buffer = c2k::MessageBuffer{};
    buffer << static_cast<std::uint8_t>(MessageType::Heartbeat) << std::uint16_t{ 10 } << std::uint8_t{ 1 }
           << std::uint8_t{ 2 } << std::uint8_t{ 3 };

    EXPECT_THROW({std::ignore = send_receive_buffer_and_deserialize(buffer);}, c2k::TimeoutError);
}

// TEST(NetworkTests, GridStateMessage) {
//
// }
