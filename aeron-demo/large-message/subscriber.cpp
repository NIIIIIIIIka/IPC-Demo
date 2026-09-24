#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "Aeron.h"
#include "FragmentAssembler.h"

using namespace aeron;

namespace
{

constexpr std::int32_t STREAM_ID = 2002;
constexpr std::size_t EXPECTED_MESSAGE_SIZE = 1024 * 1024;
constexpr int MESSAGE_COUNT = 10;
constexpr int POLL_FRAGMENT_LIMIT = 100;

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

bool verifyPayload(AtomicBuffer &buffer, util::index_t length)
{
    if (static_cast<std::size_t>(length) != EXPECTED_MESSAGE_SIZE)
    {
        std::cerr << "unexpected message length: " << length << '\n';
        return false;
    }

    const auto *data = buffer.buffer();
    const std::int64_t seq = buffer.getInt64(0);

    for (std::size_t j = 8; j < EXPECTED_MESSAGE_SIZE; ++j)
    {
        if (data[j] != static_cast<std::uint8_t>((j + seq) & 0xFF))
        {
            std::cerr << "payload mismatch at byte " << j << " of message #" << seq << '\n';
            return false;
        }
    }
    return true;
}

} // namespace

int main()
{
    const std::string channel = "aeron:ipc";

    Context context;
    auto aeron = Aeron::connect(context);

    const std::int64_t registrationId = aeron->addSubscription(channel, STREAM_ID);
    auto subscription = awaitSubscription(*aeron, registrationId);

    int received = 0;

    fragment_handler_t reassembledHandler =
        [&](AtomicBuffer &buffer, util::index_t offset, util::index_t length, Header &header)
        {
            const auto start = std::chrono::steady_clock::now();

            AtomicBuffer view(buffer.buffer() + offset, static_cast<std::size_t>(length));
            const bool ok = verifyPayload(view, length);

            const auto end = std::chrono::steady_clock::now();
            const std::chrono::duration<double, std::milli> ms = end - start;

            std::cout << "recv: reassembled message (session=" << header.sessionId()
                      << ", length=" << length << "), verify="
                      << (ok ? "OK" : "FAILED") << ", check took " << ms.count() << " ms\n";

            if (ok)
            {
                ++received;
            }
        };

    // 大消息会被 Aeron 拆成多个 fragment，必须用 FragmentAssembler 重组
    FragmentAssembler assembler(reassembledHandler, EXPECTED_MESSAGE_SIZE);

    std::cout << "Large-message subscriber started (stream=" << STREAM_ID << ")\n";

    while (received < MESSAGE_COUNT)
    {
        const int fragments = subscription->poll(assembler.handler(), POLL_FRAGMENT_LIMIT);
        if (fragments == 0)
        {
            std::this_thread::yield();
        }
    }

    std::cout << "all " << received << " large messages received and verified\n";
}
