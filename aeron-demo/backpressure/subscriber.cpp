#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <thread>

#include "Aeron.h"

namespace
{
constexpr std::int32_t STREAM_ID = 4101;
std::shared_ptr<aeron::Subscription> await_subscription(aeron::Aeron& client, std::int64_t id)
{
    std::shared_ptr<aeron::Subscription> result;
    while (!(result = client.findSubscription(id))) std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return result;
}
}

int main()
{
    aeron::Context context;
    auto client = aeron::Aeron::connect(context);
    auto subscription = await_subscription(*client, client->addSubscription("aeron:ipc", STREAM_ID));
    std::uint64_t received = 0;
    aeron::fragment_handler_t handler = [&](const aeron::AtomicBuffer&, aeron::util::index_t,
                                             aeron::util::index_t length, const aeron::Header&)
    {
        ++received;
        if (received % 100 == 0)
            std::cout << "[Subscriber] 已接收 " << received << " 条，最近一条=" << length << "B\n";
    };

    std::cout << "[Subscriber] Subscription 就绪；每 500ms 才 poll 一次，故意制造慢消费者\n";
    while (true)
    {
        subscription->poll(handler, 10);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
