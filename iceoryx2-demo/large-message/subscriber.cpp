#include "iox2/iceoryx2.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>

namespace
{

constexpr std::uint64_t MESSAGE_SIZE = 1024 * 1024;
constexpr std::uint64_t MESSAGE_COUNT = 10;
constexpr iox2::bb::Duration WAIT_TIME = iox2::bb::Duration::from_millis(10);

bool verify(const iox2::bb::Slice<const std::uint8_t>& payload, std::uint64_t sequence)
{
    if (payload.number_of_bytes() != MESSAGE_SIZE)
    {
        return false;
    }

    for (std::size_t index = 0; index < MESSAGE_SIZE; ++index)
    {
        if (payload[index] != static_cast<std::uint8_t>((index + sequence) & 0xFFU))
        {
            return false;
        }
    }
    return true;
}

} // namespace

auto main() -> int
{
    using namespace iox2;

    set_log_level_from_env_or(LogLevel::Info);

    auto node = NodeBuilder().create<ServiceType::Ipc>().value();
    auto service = node
                       .service_builder(ServiceName::create("ipc-demo/iceoryx2/large-message").value())
                       .publish_subscribe<bb::Slice<std::uint8_t>>()
                       .open_or_create()
                       .value();
    auto subscriber = service.subscriber_builder().create().value();

    std::cout << "iceoryx2 large-message subscriber ready\n";

    std::uint64_t sequence = 0;
    while (sequence < MESSAGE_COUNT && node.wait(WAIT_TIME).has_value())
    {
        auto sample = subscriber.receive().value();
        while (sample.has_value())
        {
            const auto start = std::chrono::steady_clock::now();
            const auto payload = sample->payload();
            const auto received_sequence = static_cast<std::uint64_t>(payload[0]);
            const bool valid = verify(payload, received_sequence);
            const auto elapsed = std::chrono::steady_clock::now() - start;

            std::cout << "recv: seq=" << received_sequence
                      << ", bytes=" << payload.number_of_bytes()
                      << ", subscriber virtual address=" << static_cast<const void*>(payload.data())
                      << ", verify=" << (valid ? "OK" : "FAILED")
                      << ", check="
                      << std::chrono::duration<double, std::micro>(elapsed).count()
                      << " us\n";

            if (!valid)
            {
                return 1;
            }

            ++sequence;
            sample = subscriber.receive().value();
        }
    }

    std::cout << "received and verified " << sequence << " samples\n";
    return sequence == MESSAGE_COUNT ? 0 : 1;
}
