#pragma once

#include <cstdint>
#include <sockets/sockets.hpp>
#include "message_types.hpp"

using MessageSize = std::uint16_t;

struct MessageHeader {
    MessageType type;
    MessageSize payload_size;

    friend c2k::MessageBuffer& operator<<(c2k::MessageBuffer& buffer, MessageHeader const header) {
        return buffer << std::to_underlying(header.type) << header.payload_size;
    }

    friend c2k::MessageBuffer& operator>>(c2k::MessageBuffer& buffer, MessageHeader& header) {
        auto const tuple = buffer.try_extract<std::underlying_type_t<MessageType>, MessageSize>();
        if (not tuple.has_value()) {
            throw std::invalid_argument{ "not enough data to extract message header" };
        }
        auto const [type, payload_size] = tuple.value();
        auto const message_type = magic_enum::enum_cast<MessageType>(type);
        if (not message_type.has_value()) {
            throw std::invalid_argument{ std::format("{} is not a valid message type", type) };
        }

        header.type = message_type.value();
        header.payload_size = payload_size;
        return buffer;
    }
};
