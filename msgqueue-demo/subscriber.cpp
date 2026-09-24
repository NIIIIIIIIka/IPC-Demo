#include <iostream>
#include <string>
#include <sys/msg.h>

struct Msg {
    long type;
    char text[256];
};

int main()
{
    const long key = 12345;

    int qid = msgget(key, 0666);
    if (qid < 0) {
        std::cerr << "msgget failed, publisher not run yet?" << std::endl;
        return 1;
    }

    std::cout << "Waiting for message..." << std::endl;

    Msg msg{};
    if (msgrcv(qid, &msg, sizeof(msg.text), 1, 0) < 0) {
        std::cerr << "msgrcv failed" << std::endl;
        return 1;
    }

    std::string message(msg.text);
    std::cout << "recv: " << message << std::endl;

    // 清理队列
    msgctl(qid, IPC_RMID, nullptr);
    return 0;
}
