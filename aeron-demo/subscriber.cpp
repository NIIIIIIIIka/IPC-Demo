#include <iostream>
#include <thread>
#include <memory>
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
        aeron->addSubscription(channel, streamId);

    std::shared_ptr<Subscription> subscription;

    while (!subscription)
    {
        subscription = aeron->findSubscription(registrationId);
        std::this_thread::yield();
    }

    std::cout << "Subscriber started\n";

    fragment_handler_t handler =
        [](const AtomicBuffer &buffer,
           util::index_t offset,
           util::index_t length,
           const Header &)
        {
            std::string message(
                reinterpret_cast<const char *>(buffer.buffer()) + offset,
                length);

            std::cout << "recv: " << message << std::endl;
        };

    while (true)
    {
        subscription->poll(handler, 10);
        std::this_thread::yield();
    }
}