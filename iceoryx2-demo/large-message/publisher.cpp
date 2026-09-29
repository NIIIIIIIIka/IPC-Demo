#include "iox2/iceoryx2.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <thread>
#include <utility>

namespace
{

constexpr std::uint64_t MESSAGE_SIZE = 1024 * 1024;
constexpr std::uint64_t MESSAGE_COUNT = 10;

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
    auto publisher = service
                         .publisher_builder()
                         .initial_max_slice_len(MESSAGE_SIZE)
                         .allocation_strategy(AllocationStrategy::Static)
                         .create()
                         .value();

    std::cout << "iceoryx2 large-message publisher started\n";
    std::cout << "payload size: " << MESSAGE_SIZE << " bytes\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));

    for (std::uint64_t sequence = 0; sequence < MESSAGE_COUNT; ++sequence)
    {
        const auto start = std::chrono::steady_clock::now();
        auto sample = publisher.loan_slice_uninit(MESSAGE_SIZE).value();
        auto initialized = sample.write_from_fn([&](std::size_t index) {
            return static_cast<std::uint8_t>((index + sequence) & 0xFFU);
        });
        const auto* payload_address = initialized.payload().data();
        send(std::move(initialized)).value();
        const auto elapsed = std::chrono::steady_clock::now() - start;

        std::cout << "send: seq=" << sequence
                  << ", bytes=" << MESSAGE_SIZE
                  << ", publisher virtual address=" << static_cast<const void*>(payload_address)
                  << ", fill+send="
                  << std::chrono::duration<double, std::micro>(elapsed).count()
                  << " us\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "done\n";
    return 0;
}
