#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "Aeron.h"

using namespace aeron;

namespace
{
constexpr std::int32_t STREAM_ID = 2001;
constexpr std::size_t DEFAULT_MESSAGE_SIZE = 256;
constexpr int DEFAULT_MESSAGE_COUNT = 1'000'000;

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

std::shared_ptr<Publication> awaitPublication(Aeron &aeron, std::int64_t id)
{
    std::shared_ptr<Publication> publication;
    while (!publication)
    {
        publication = aeron.findPublication(id);
        std::this_thread::yield();
    }
    return publication;
}
} // namespace

int main(int argc, char **argv)
{
    if (argc > 4 || (argc > 1 && std::string(argv[1]) == "--help"))
    {
        std::cout << "Usage: ./publisher [message-count=1000000] [message-size=256] [rate-msg/s=0]\n"
                  << "rate 0 means send as fast as possible; size must be 1.." << (1024 * 1024) << " bytes.\n";
        return argc > 4 ? 2 : 0;
    }
    const long countArg = argc > 1 ? parseLong(argv[1], "count") : DEFAULT_MESSAGE_COUNT;
    const long sizeArg = argc > 2 ? parseLong(argv[2], "size") : DEFAULT_MESSAGE_SIZE;
    const long rate = argc > 3 ? parseLong(argv[3], "rate") : 0;
    if (countArg <= 0 || sizeArg <= 0 || sizeArg > 1024 * 1024 || rate < 0)
    {
        std::cerr << "count and size must be positive, size <= 1 MiB, rate >= 0\n";
        return 2;
    }
    const auto messageCount = static_cast<std::uint64_t>(countArg);
    const auto messageSize = static_cast<std::size_t>(sizeArg);
    const std::string channel = "aeron:ipc";

    Context context;
    auto aeron = Aeron::connect(context);
    const auto registrationId = aeron->addPublication(channel, STREAM_ID);
    auto publication = awaitPublication(*aeron, registrationId);

    std::cout << "Streaming publisher (stream=" << STREAM_ID << ", bytes=" << messageSize
              << ", target-rate=" << rate << " msg/s)\nWaiting for subscriber...\n";
    while (!publication->isConnected()) std::this_thread::yield();

    std::vector<std::uint8_t> payload(messageSize, 0);
    concurrent::AtomicBuffer buffer(payload.data(), static_cast<util::index_t>(payload.size()));
    std::int64_t backPressure = 0, notConnected = 0, adminAction = 0;
    std::uint64_t sent = 0;
    const auto start = std::chrono::steady_clock::now();
    const auto interval = rate > 0 ? std::chrono::duration<double>(1.0 / rate)
                                   : std::chrono::duration<double>::zero();

    while (sent < messageCount)
    {
        const auto now = std::chrono::steady_clock::now();
        if (rate > 0)
        {
            const auto deadline = start + std::chrono::duration_cast<std::chrono::steady_clock::duration>(interval * (sent + 1));
            if (now < deadline) std::this_thread::sleep_until(deadline);
        }
        const std::string text = "msg-" + std::to_string(sent);
        const auto copyLength = std::min(text.size(), messageSize);
        buffer.putBytes(0, reinterpret_cast<const std::uint8_t *>(text.data()), copyLength);
        const auto result = publication->offer(buffer, 0, static_cast<util::index_t>(messageSize));
        if (result > 0) { ++sent; continue; }
        switch (result)
        {
            case AERON_PUBLICATION_BACK_PRESSURED: ++backPressure; break;
            case AERON_PUBLICATION_NOT_CONNECTED: ++notConnected; break;
            case AERON_PUBLICATION_ADMIN_ACTION: ++adminAction; break;
            case AERON_PUBLICATION_CLOSED: std::cerr << "publication closed\n"; return 1;
            default: break;
        }
        std::this_thread::yield();
    }

    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::cout << "sent: " << sent << " messages in " << seconds << " s\n"
              << "throughput: " << (sent / seconds) << " msg/s, "
              << ((sent * messageSize) / (1024.0 * 1024.0) / seconds) << " MiB/s\n"
              << "back-pressure: " << backPressure << ", not-connected: " << notConnected
              << ", admin-action: " << adminAction << '\n';
}
