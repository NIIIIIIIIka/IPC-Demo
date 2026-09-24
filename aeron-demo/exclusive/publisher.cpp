#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "Aeron.h"
#include "concurrent/logbuffer/BufferClaim.h"

using namespace aeron;

namespace
{

constexpr std::int32_t STREAM_ID_A = 3001;
constexpr std::int32_t STREAM_ID_B = 3002;
constexpr int MESSAGE_COUNT_PER_STREAM = 1000;

std::shared_ptr<ExclusivePublication> awaitExclusivePublication(
    Aeron &aeron, std::int64_t registrationId)
{
    std::shared_ptr<ExclusivePublication> publication;
    while (!publication)
    {
        publication = aeron.findExclusivePublication(registrationId);
        std::this_thread::yield();
    }
    return publication;
}

void sendStream(
    ExclusivePublication &publication,
    std::int32_t streamId,
    const std::string &tag)
{
    std::cout << "Waiting for subscriber on stream " << streamId << "...\n";
    while (!publication.isConnected())
    {
        std::this_thread::yield();
    }

    int sent = 0;
    std::int64_t backPressure = 0;
    std::int64_t windowBytes = 0;

    while (sent < MESSAGE_COUNT_PER_STREAM)
    {
        const std::string text = tag + "-" + std::to_string(sent);
        const auto length = static_cast<util::index_t>(text.size());

        concurrent::logbuffer::BufferClaim claim;
        const std::int64_t result = publication.tryClaim(length, claim);

        if (result > 0)
        {
            // 零拷贝：直接在 claim 到的 buffer 里写，commit 后才对订阅者可见
            claim.buffer().putBytes(
                claim.offset(),
                reinterpret_cast<const std::uint8_t *>(text.data()),
                text.size());
            claim.commit();
            ++sent;
            continue;
        }

        if (result == AERON_PUBLICATION_BACK_PRESSURED)
        {
            ++backPressure;
            windowBytes = publication.availableWindow();
        }
        else if (result == AERON_PUBLICATION_CLOSED)
        {
            std::cerr << "publication closed on stream " << streamId << '\n';
            return;
        }

        std::this_thread::yield();
    }

    std::cout << "stream " << streamId << ": sent " << sent
              << " messages, back-pressure=" << backPressure
              << ", available-window=" << windowBytes << " bytes\n";
}

} // namespace

int main()
{
    const std::string channel = "aeron:ipc";

    Context context;
    auto aeron = Aeron::connect(context);

    // ExclusivePublication：一个 stream 只能有一个独占发布者，但可提供更低延迟
    const std::int64_t regA = aeron->addExclusivePublication(channel, STREAM_ID_A);
    const std::int64_t regB = aeron->addExclusivePublication(channel, STREAM_ID_B);

    auto pubA = awaitExclusivePublication(*aeron, regA);
    auto pubB = awaitExclusivePublication(*aeron, regB);

    std::cout << "Exclusive publisher started (streams=" << STREAM_ID_A
              << "," << STREAM_ID_B << ")\n";

    // 交错发送两个 stream，模拟单进程多路复用
    std::thread threadA(sendStream, std::ref(*pubA), STREAM_ID_A, "alpha");
    std::thread threadB(sendStream, std::ref(*pubB), STREAM_ID_B, "beta");

    threadA.join();
    threadB.join();

    std::cout << "done\n";
}
