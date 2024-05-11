#pragma once

#include <memory>
#include <obpf/simulator.h>
#include <cstdint>

class Tetrion final {
private:
    struct Deleter {
        void operator()(ObpfTetrion const* handle);
    };

    std::unique_ptr<ObpfTetrion, Deleter> m_handle;

public:
    Tetrion();
    void enqueue_event(ObpfEvent event);
    void simulate_up_until(std::uint64_t frame);
};
