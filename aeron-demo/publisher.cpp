#include <iostream>
#include <thread>
#include <memory>
#include <array>
#include <string>

#include "Aeron.h"

using namespace aeron;

int main()
{
    const std::string channel = "aeron:ipc";
    const std::int32_t streamId = 1001;

    Context context;
    auto aeron = Aeron::connect(context);

    auto registrationId =
        aeron->addPublication(channel, streamId);

    std::shared_ptr<Publication> publication;

    while (!publication)
    {
        publication = aeron->findPublication(registrationId);
        std::this_thread::yield();
    }

    std::cout << "Publisher started\n";
    std::cout << "Waiting for subscriber...\n";

    while (!publication->isConnected())
    {
        std::this_thread::yield();
    }

    std::string message = "Hello Aeron";

    alignas(16) std::array<std::uint8_t, 256> data{};

    concurrent::AtomicBuffer buffer(
        data.data(),
        data.size());

    buffer.putBytes(
        0,
        reinterpret_cast<const std::uint8_t *>(message.data()),
        message.size());

    while (publication->offer(
               buffer,
               0,
               static_cast<util::index_t>(message.size())) < 0)
    {
        std::this_thread::yield();
    }

    std::cout << "send: " << message << std::endl;
}