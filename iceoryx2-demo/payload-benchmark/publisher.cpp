#include "iox2/iceoryx2.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>
#include <utility>

int main(int argc, char** argv)
{
    using namespace iox2;
    const std::size_t payload_size = argc > 1 ? std::stoull(argv[1]) : 64;
    const std::uint64_t count = argc > 2 ? std::stoull(argv[2]) : 200;
    const std::string run_id = argc > 3 ? argv[3] : "default";

    set_log_level_from_env_or(LogLevel::Warn);
    auto node = NodeBuilder().create<ServiceType::Ipc>().value();
    auto service = node.service_builder(
        ServiceName::create(("ipc-demo/payload-benchmark/" + run_id).c_str()).value())
        .publish_subscribe<bb::Slice<std::uint8_t>>()
        .open_or_create().value();
    auto publisher = service.publisher_builder()
        .initial_max_slice_len(payload_size)
        .allocation_strategy(AllocationStrategy::Static)
        .create().value();

    std::cout << "[Publisher] payload=" << payload_size << "B, count=" << count << '\n';
    std::this_thread::sleep_for(std::chrono::seconds(1));
    double fill_us = 0.0, send_us = 0.0;
    for (std::uint64_t sequence = 0; sequence < count; ++sequence)
    {
        auto sample = publisher.loan_slice_uninit(payload_size).value();
        const auto fill_start = std::chrono::steady_clock::now();
        auto initialized = sample.write_from_fn([sequence](std::size_t i) {
            return static_cast<std::uint8_t>((i + sequence) & 0xFFU);
        });
        const auto send_start = std::chrono::steady_clock::now();
        send(std::move(initialized)).value();
        const auto finished = std::chrono::steady_clock::now();
        fill_us += std::chrono::duration<double, std::micro>(send_start - fill_start).count();
        send_us += std::chrono::duration<double, std::micro>(finished - send_start).count();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    std::cout << "[Publisher] 平均原地填充=" << fill_us / count
              << "us，平均 send(句柄提交)=" << send_us / count << "us\n";
}
