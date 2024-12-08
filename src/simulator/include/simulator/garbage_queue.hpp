#pragma once

#include <deque>
#include <lib2k/synchronized.hpp>
#include <optional>
#include "garbage.hpp"

class GarbageQueue final {
private:
    c2k::Synchronized<std::deque<GarbageSendEvent>> m_queue;

public:
    GarbageQueue() = default;

    GarbageQueue(GarbageQueue const& other)
        : m_queue{ other.m_queue.apply([](std::deque<GarbageSendEvent> const& queue) { return queue; }) } {}

    GarbageQueue& operator=(GarbageQueue const& other) {
        if (this == std::addressof(other)) {
            return *this;
        }
        m_queue.apply([&](std::deque<GarbageSendEvent>& queue) {
            queue = other.m_queue.apply([](std::deque<GarbageSendEvent> const& other_queue) { return other_queue; });
        });
        return *this;
    }

    GarbageQueue(GarbageQueue&& other) noexcept = delete;
    GarbageQueue& operator=(GarbageQueue&& other) noexcept = delete;
    ~GarbageQueue() = default;

    void enqueue(GarbageSendEvent const event) {
        m_queue.apply([&](std::deque<GarbageSendEvent>& queue) { queue.push_front(event); });
    }

    [[nodiscard]] std::optional<GarbageSendEvent> dequeue() {
        return m_queue.apply([&](std::deque<GarbageSendEvent>& queue) -> std::optional<GarbageSendEvent> {
            if (queue.empty()) {
                return std::nullopt;
            }
            auto const result = queue.back();
            queue.pop_back();
            return result;
        });
    }

    template<typename Predicate>
    [[nodiscard]] std::optional<GarbageSendEvent> dequeue(Predicate const& predicate) {
        return m_queue.apply([&](std::deque<GarbageSendEvent>& queue) -> std::optional<GarbageSendEvent> {
            if (queue.empty()) {
                return std::nullopt;
            }
            auto const result = queue.back();
            if (not predicate(result)) {
                return std::nullopt;
            }
            queue.pop_back();
            return result;
        });
    }
};
