#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

#include "Aeron.h"

using namespace aeron;

namespace
{
constexpr std::int32_t STREAM_ID = 2001;
constexpr int POLL_FRAGMENT_LIMIT = 100;

long parseLong(const char *value, const char *name)
{
    try
    {
        std::size_t used = 0;
        const long parsed = std::stol(value, &used);
        if (value[used] != '\0') throw std::invalid_argument("trailing characters");
        return parsed;
    }
    catch (const std::exception &)
    {
        std::cerr << "invalid " << name << ": " << value << '\n';
        std::exit(2);
    }
}

std::shared_ptr<Subscription> awaitSubscription(Aeron &aeron, std::int64_t id)
{
    std::shared_ptr<Subscription> subscription;
    while (!subscription)
    {
        subscription = aeron.findSubscription(id);
        std::this_thread::yield();
    }
    return subscription;
}
} // namespace

int main(int argc, char **argv)
{
    if (argc > 3 || (argc > 1 && std::string(argv[1]) == "--help"))
    {
        std::cout << "Usage: ./subscriber [delay-us=0] [label=consumer]\n"
                  << "A positive delay sleeps after each received fragment to simulate a slow consumer.\n";
        return argc > 3 ? 2 : 0;
    }
    const long delayUs = argc > 1 ? parseLong(argv[1], "delay-us") : 0;
    const std::string label = argc > 2 ? argv[2] : "consumer";
    if (delayUs < 0 || delayUs > 10'000'000)
    {
        std::cerr << "delay-us must be between 0 and 10000000\n";
        return 2;
    }

    Context context;
    auto aeron = Aeron::connect(context);
    const auto registrationId = aeron->addSubscription("aeron:ipc", STREAM_ID);
    auto subscription = awaitSubscription(*aeron, registrationId);
    std::uint64_t messages = 0, bytes = 0, lastMessages = 0, lastBytes = 0;
    auto lastReport = std::chrono::steady_clock::now();

    fragment_handler_t handler = [&](AtomicBuffer &buffer, util::index_t offset, util::index_t length, Header &)
    {
        (void)buffer; (void)offset;
        ++messages;
        bytes += static_cast<std::uint64_t>(length);
        if (delayUs > 0) std::this_thread::sleep_for(std::chrono::microseconds(delayUs));
        if (messages % 100'000 == 0)
        {
            const auto now = std::chrono::steady_clock::now();
            const double seconds = std::chrono::duration<double>(now - lastReport).count();
            if (seconds > 0.0)
                std::cout << "subscriber '" << label << "': " << messages
                          << " messages, " << ((messages - lastMessages) / seconds) << " msg/s, "
                          << ((bytes - lastBytes) / (1024.0 * 1024.0) / seconds) << " MiB/s\n" << std::flush;
            lastMessages = messages; lastBytes = bytes; lastReport = now;
        }
    };

    std::cout << "Streaming subscriber '" << label << "' started (stream=" << STREAM_ID
              << ", delay-us=" << delayUs << ")\n";
    while (true)
    {
        if (subscription->poll(handler, POLL_FRAGMENT_LIMIT) == 0) std::this_thread::yield();
    }
}
