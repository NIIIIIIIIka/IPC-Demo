#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "Aeron.h"

namespace
{
constexpr std::int32_t STREAM_ID = 4101;

std::shared_ptr<aeron::Publication> await_publication(aeron::Aeron& client, std::int64_t id)
{
    std::shared_ptr<aeron::Publication> result;
    while (!(result = client.findPublication(id))) std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return result;
}
}

int main(int argc, char** argv)
{
    // 较大的默认消息能在现场演示时快速填满 Log Buffer，清晰触发背压。
    const std::uint64_t message_count = argc > 1 ? std::stoull(argv[1]) : 4000;
    const std::size_t payload_size = argc > 2 ? std::stoull(argv[2]) : 64 * 1024;

    aeron::Context context;
    auto client = aeron::Aeron::connect(context);
    auto publication = await_publication(*client, client->addPublication("aeron:ipc", STREAM_ID));
    std::cout << "[Publisher] Publication 就绪，等待订阅者...\n";
    while (!publication->isConnected()) std::this_thread::yield();

    std::vector<std::uint8_t> payload(payload_size, 0xAB);
    aeron::concurrent::AtomicBuffer buffer(payload.data(), static_cast<aeron::util::index_t>(payload.size()));
    std::uint64_t sent = 0, back_pressured = 0, other_retries = 0;
    const auto started = std::chrono::steady_clock::now();
    while (sent < message_count)
    {
        const auto result = publication->offer(buffer, 0, static_cast<aeron::util::index_t>(payload.size()));
        if (result > 0)
        {
            ++sent;
            if (sent % 250 == 0) std::cout << "[Publisher] 已发送 " << sent << " 条\n";
        }
        else if (result == AERON_PUBLICATION_BACK_PRESSURED)
        {
            ++back_pressured;
            if (back_pressured == 1 || back_pressured % 10000 == 0)
                std::cout << "[Publisher] 背压 (BACK_PRESSURED)，累计 " << back_pressured << " 次\n";
            std::this_thread::yield();
        }
        else if (result == AERON_PUBLICATION_CLOSED || result == AERON_PUBLICATION_MAX_POSITION_EXCEEDED)
        {
            std::cerr << "[Publisher] Publication 已关闭或超过最大位置，返回=" << result << '\n';
            return 1;
        }
        else
        {
            ++other_retries;
            std::this_thread::yield();
        }
    }

    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    std::cout << "[Publisher] 完成：发送=" << sent << "，背压=" << back_pressured
              << "，其他重试=" << other_retries << "，耗时=" << seconds << "s\n";
}
