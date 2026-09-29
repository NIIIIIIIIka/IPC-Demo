#include "iox2/iceoryx2.hpp"
#include "transmission_data.hpp"

#include <cstdint>
#include <iostream>

namespace
{

constexpr std::uint64_t MESSAGE_COUNT = 10;
constexpr iox2::bb::Duration WAIT_TIME = iox2::bb::Duration::from_millis(100);

} // namespace

auto main() -> int
{
    using namespace iox2;

    set_log_level_from_env_or(LogLevel::Info);

    auto node = NodeBuilder().create<ServiceType::Ipc>().value();
    auto service = node
                       .service_builder(ServiceName::create("ipc-demo/iceoryx2/basic").value())
                       .publish_subscribe<TransmissionData>()
                       .open_or_create()
                       .value();
    auto subscriber = service.subscriber_builder().create().value();

    std::cout << "iceoryx2 basic subscriber ready\n";

    std::uint64_t received = 0;
    while (received < MESSAGE_COUNT && node.wait(WAIT_TIME).has_value())
    {
        auto sample = subscriber.receive().value();
        while (sample.has_value())
        {
            const auto& payload = sample->payload();
            std::cout << "recv: seq=" << payload.sequence
                      << ", value=" << payload.value
                      << ", temperature=" << payload.temperature
                      << ", subscriber virtual address=" << &payload << '\n';
            ++received;
            sample = subscriber.receive().value();
        }
    }

    std::cout << "received " << received << " samples\n";
    return received == MESSAGE_COUNT ? 0 : 1;
}

