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

constexpr std::int32_t STREAM_ID = 2001;
constexpr std::size_t MESSAGE_SIZE = 256;
constexpr int DEFAULT_MESSAGE_COUNT = 1'000'000;

int parseCount(int argc, char **argv)
{
    if (argc > 1)
    {
        try
        {
            return std::stoi(argv[1]);
        }
        catch (const std::exception &)
        {
            std::cerr << "invalid count, using default " << DEFAULT_MESSAGE_COUNT << '\n';
        }
    }
    return DEFAULT_MESSAGE_COUNT;
}

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

int main(int argc, char **argv)
{
    const int messageCount = parseCount(argc, argv);
    const std::string channel = "aeron:ipc";

    Context context;
    auto aeron = Aeron::connect(context);

    const std::int64_t registrationId = aeron->addPublication(channel, STREAM_ID);
    auto publication = awaitPublication(*aeron, registrationId);

    std::cout << "Streaming publisher started (stream=" << STREAM_ID << ")\n";
    std::cout << "Waiting for subscriber...\n";
    while (!publication->isConnected())
    {
        std::this_thread::yield();
    }

    std::vector<std::uint8_t> payload(MESSAGE_SIZE);
    concurrent::AtomicBuffer buffer(payload.data(), payload.size());

    std::int64_t backPressureCount = 0;
    std::int64_t notConnectedCount = 0;
    std::int64_t adminActionCount = 0;
    int sent = 0;

    const auto start = std::chrono::steady_clock::now();

    while (sent < messageCount)
    {
        const std::string text = "msg-" + std::to_string(sent);
        const auto copyLength = std::min(text.size(), MESSAGE_SIZE);
        buffer.putBytes(0, reinterpret_cast<const std::uint8_t *>(text.data()), copyLength);

        const std::int64_t result = publication->offer(
            buffer, 0, static_cast<util::index_t>(MESSAGE_SIZE));

        if (result > 0)
        {
            ++sent;
            continue;
        }

        switch (result)
        {
            case AERON_PUBLICATION_BACK_PRESSURED:
                ++backPressureCount;
                break;
            case AERON_PUBLICATION_NOT_CONNECTED:
                ++notConnectedCount;
                break;
            case AERON_PUBLICATION_ADMIN_ACTION:
                ++adminActionCount;
                break;
            case AERON_PUBLICATION_CLOSED:
                std::cerr << "publication closed\n";
                return 1;
            default:
                break;
        }

        std::this_thread::yield();
    }

    const auto end = std::chrono::steady_clock::now();
    const std::chrono::duration<double> elapsed = end - start;
    const double seconds = elapsed.count();
    const double messagesPerSecond = messageCount / seconds;
    const double megabytesPerSecond = (messageCount * MESSAGE_SIZE) / (1024.0 * 1024.0) / seconds;

    std::cout << "sent: " << messageCount << " messages in " << seconds << " s\n";
    std::cout << "throughput: " << messagesPerSecond << " msg/s, "
              << megabytesPerSecond << " MB/s\n";
    std::cout << "back-pressure: " << backPressureCount
              << ", not-connected: " << notConnectedCount
              << ", admin-action: " << adminActionCount << '\n';
}
