#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "Aeron.h"

using namespace aeron;

namespace
{

constexpr std::int32_t STREAM_ID_A = 3001;
constexpr std::int32_t STREAM_ID_B = 3002;
constexpr int MESSAGE_COUNT_PER_STREAM = 1000;
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

class StreamState
{
public:
    explicit StreamState(std::string tag) : m_tag(std::move(tag)) {}

    void onMessage(const std::string &message)
    {
        std::cout << "recv[" << m_tag << "]: " << message << '\n';
        ++m_count;
    }

    bool complete() const { return m_count >= MESSAGE_COUNT_PER_STREAM; }
    int count() const { return m_count; }

private:
    std::string m_tag;
    int m_count = 0;
};

void pollStream(Subscription &subscription, StreamState &state)
{
    fragment_handler_t handler =
        [&](AtomicBuffer &buffer, util::index_t offset, util::index_t length, Header &)
        {
            std::string message(
                reinterpret_cast<const char *>(buffer.buffer() + offset),
                static_cast<std::size_t>(length));
            state.onMessage(message);
        };

    while (!state.complete())
    {
        const int fragments = subscription.poll(handler, POLL_FRAGMENT_LIMIT);
        if (fragments == 0)
        {
            std::this_thread::yield();
        }
    }
}

} // namespace

int main()
{
    const std::string channel = "aeron:ipc";

    Context context;
    auto aeron = Aeron::connect(context);

    const std::int64_t regA = aeron->addSubscription(channel, STREAM_ID_A);
    const std::int64_t regB = aeron->addSubscription(channel, STREAM_ID_B);

    auto subA = awaitSubscription(*aeron, regA);
    auto subB = awaitSubscription(*aeron, regB);

    StreamState stateA("alpha");
    StreamState stateB("beta");

    std::cout << "Exclusive subscriber started (streams=" << STREAM_ID_A
              << "," << STREAM_ID_B << ")\n";

    std::thread threadA(pollStream, std::ref(*subA), std::ref(stateA));
    std::thread threadB(pollStream, std::ref(*subB), std::ref(stateB));

    threadA.join();
    threadB.join();

    std::cout << "alpha: " << stateA.count() << ", beta: " << stateB.count() << '\n';
}
