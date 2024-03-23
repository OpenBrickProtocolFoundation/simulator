#pragma once

#include <future>
#include <network/messages.hpp>
#include <sockets/sockets.hpp>

[[nodiscard]] inline std::unique_ptr<AbstractMessage> send_receive_buffer_and_deserialize(
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
    assert(num_bytes_sent == buffer.size());
    auto result = future.get();
    return result;
}

[[nodiscard]] inline std::unique_ptr<AbstractMessage> send_receive_and_deserialize(AbstractMessage const& message) {
    auto const buffer = message.serialize();
    return send_receive_buffer_and_deserialize(buffer);
}
