#include "iox2/iceoryx2.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>

int main(int argc, char** argv)
{
    using namespace iox2;
    const std::size_t expected_size = argc > 1 ? std::stoull(argv[1]) : 64;
    const std::uint64_t expected_count = argc > 2 ? std::stoull(argv[2]) : 200;
    const std::string run_id = argc > 3 ? argv[3] : "default";

    set_log_level_from_env_or(LogLevel::Warn);
    auto node = NodeBuilder().create<ServiceType::Ipc>().value();
    auto service = node.service_builder(
        ServiceName::create(("ipc-demo/payload-benchmark/" + run_id).c_str()).value())
        .publish_subscribe<bb::Slice<std::uint8_t>>()
        .open_or_create().value();
    auto subscriber = service.subscriber_builder().create().value();
    std::cout << "[Subscriber] 等待 " << expected_count << " 条 " << expected_size << "B 样本\n";

    std::uint64_t received = 0;
    while (received < expected_count && node.wait(bb::Duration::from_millis(10)).has_value())
    {
        auto sample = subscriber.receive().value();
        while (sample.has_value())
        {
            if (sample->payload().number_of_bytes() != expected_size)
            {
                std::cerr << "[Subscriber] payload 大小不符\n";
                return 1;
            }
            // 直接读取共享内存视图的首尾字节，不复制 payload。
            volatile std::uint8_t observed = sample->payload()[0] ^ sample->payload()[expected_size - 1];
            (void)observed;
            ++received;
            sample = subscriber.receive().value();
        }
    }
    std::cout << "[Subscriber] 已零拷贝接收 " << received << " 条样本\n";
    return received == expected_count ? 0 : 1;
}
