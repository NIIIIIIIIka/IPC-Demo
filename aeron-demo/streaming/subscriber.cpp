#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "Aeron.h"

using namespace aeron;

namespace
{

constexpr std::int32_t STREAM_ID = 2001;
constexpr int POLL_FRAGMENT_LIMIT = 100;
constexpr std::size_t REPORT_INTERVAL = 1'000'000;

std::shared_ptr<Subscription> awaitSubscription(Aeron &aeron, std::int64_t registrationId)
{
    std::shared_ptr<Subscription> subscription;
    while (!subscription)
    {
        subscription = aeron.findSubscription(registrationId);
        std::this_thread::yield();
    }
    return subscription;
}

} // namespace

int main()
{
    const std::string channel = "aeron:ipc";

    Context context;
    auto aeron = Aeron::connect(context);

    const std::int64_t registrationId = aeron->addSubscription(channel, STREAM_ID);
    auto subscription = awaitSubscription(*aeron, registrationId);

    std::atomic<std::uint64_t> messageCount{0};
    std::atomic<std::uint64_t> byteCount{0};

    const auto start = std::chrono::steady_clock::now();
    auto lastReport = start;
    std::uint64_t lastMessages = 0;
    std::uint64_t lastBytes = 0;

    fragment_handler_t handler =
        [&](AtomicBuffer &buffer, util::index_t offset, util::index_t length, Header &)
        {
            const auto now = messageCount.fetch_add(1, std::memory_order_relaxed) + 1;
            const auto bytes = byteCount.fetch_add(static_cast<std::uint64_t>(length),
                                                   std::memory_order_relaxed) +
                               static_cast<std::uint64_t>(length);

            if (now % REPORT_INTERVAL == 0)
            {
                const auto reportNow = std::chrono::steady_clock::now();
                const std::chrono::duration<double> reportElapsed = reportNow - lastReport;
                const double seconds = reportElapsed.count();
                if (seconds > 0.0)
                {
                    const std::uint64_t deltaMessages = now - lastMessages;
                    const std::uint64_t deltaBytes = bytes - lastBytes;
                    std::cout << "recv: " << now << " messages, "
                              << (deltaMessages / seconds) << " msg/s, "
                              << (deltaBytes / (1024.0 * 1024.0) / seconds) << " MB/s\n"
                              << std::flush;
                }
                lastReport = reportNow;
                lastMessages = now;
                lastBytes = bytes;
            }
        };

    std::cout << "Streaming subscriber started (stream=" << STREAM_ID << ")\n";

    while (true)
    {
        const int fragments = subscription->poll(handler, POLL_FRAGMENT_LIMIT);
        if (fragments == 0)
        {
            std::this_thread::yield();
        }
    }
}
