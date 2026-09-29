#include "iox2/iceoryx2.hpp"
#include "transmission_data.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>
#include <utility>

namespace
{

constexpr std::uint64_t MESSAGE_COUNT = 10;
constexpr auto PUBLISH_INTERVAL = std::chrono::milliseconds(200);

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
    auto publisher = service.publisher_builder().create().value();

    std::cout << "iceoryx2 basic publisher started\n";
    std::cout << "loan -> write_payload -> send (no payload memcpy)\n";

    // Give a subscriber started in the other terminal time to discover the service.
    std::this_thread::sleep_for(std::chrono::seconds(1));

    for (std::uint64_t sequence = 0; sequence < MESSAGE_COUNT; ++sequence)
    {
        auto sample = publisher.loan_uninit().value();
        auto initialized = sample.write_payload(TransmissionData {
            sequence,
            static_cast<std::int64_t>(sequence * 3),
            20.0 + static_cast<double>(sequence) * 0.25,
        });

        const auto* payload_address = &initialized.payload();
        send(std::move(initialized)).value();

        std::cout << "send: seq=" << sequence
                  << ", publisher virtual address=" << payload_address << '\n';
        std::this_thread::sleep_for(PUBLISH_INTERVAL);
    }

    std::cout << "done\n";
    return 0;
}

