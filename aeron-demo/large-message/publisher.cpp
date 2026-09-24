#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "Aeron.h"

using namespace aeron;

namespace
{

constexpr std::int32_t STREAM_ID = 2002;
constexpr std::size_t LARGE_MESSAGE_SIZE = 1024 * 1024; // 1 MiB，远大于 MTU，会触发分片
constexpr int MESSAGE_COUNT = 10;

std::shared_ptr<Publication> awaitPublication(Aeron &aeron, std::int64_t registrationId)
{
    std::shared_ptr<Publication> publication;
    while (!publication)
    {
        publication = aeron.findPublication(registrationId);
        std::this_thread::yield();
    }
    return publication;
}

} // namespace

int main()
{
    const std::string channel = "aeron:ipc";

    Context context;
    auto aeron = Aeron::connect(context);

    const std::int64_t registrationId = aeron->addPublication(channel, STREAM_ID);
    auto publication = awaitPublication(*aeron, registrationId);

    std::cout << "Large-message publisher started (stream=" << STREAM_ID << ")\n";
    std::cout << "message size: " << LARGE_MESSAGE_SIZE << " bytes (fragmented by Aeron)\n";
    std::cout << "Waiting for subscriber...\n";
    while (!publication->isConnected())
    {
        std::this_thread::yield();
    }

    std::vector<std::uint8_t> payload(LARGE_MESSAGE_SIZE);
    concurrent::AtomicBuffer buffer(payload.data(), payload.size());

    for (int i = 0; i < MESSAGE_COUNT; ++i)
    {
        // 用可验证的模式填充：前 8 字节为序号，其余按 (index + seq) 取模
        buffer.putInt64(0, i);
        for (std::size_t j = 8; j < LARGE_MESSAGE_SIZE; ++j)
        {
            payload[j] = static_cast<std::uint8_t>((j + i) & 0xFF);
        }

        while (true)
        {
            const std::int64_t result = publication->offer(
                buffer, 0, static_cast<util::index_t>(LARGE_MESSAGE_SIZE));

            if (result > 0)
            {
                std::cout << "send: large message #" << i << '\n';
                break;
            }

            if (result == AERON_PUBLICATION_CLOSED)
            {
                std::cerr << "publication closed\n";
                return 1;
            }

            // BACK_PRESSURED / NOT_CONNECTED / ADMIN_ACTION：稍后重试
            std::this_thread::yield();
        }
    }

    std::cout << "done\n";
}
