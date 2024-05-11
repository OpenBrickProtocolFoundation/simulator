#include "tetrion.hpp"
#include <obpf/simulator.h>

void Tetrion::Deleter::operator()(ObpfTetrion const* const handle) {
    obpf_destroy_tetrion(handle);
}

Tetrion::Tetrion() : m_handle{ obpf_create_tetrion() } { }

void Tetrion::simulate_up_until(std::uint64_t const frame) {
    obpf_tetrion_simulate_up_until(m_handle.get(), frame);
}

void Tetrion::enqueue_event(ObpfEvent const event) {
    obpf_tetrion_enqueue_event(m_handle.get(), event);
}
