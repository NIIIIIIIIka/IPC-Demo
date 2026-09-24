#include <iostream>
#include <string>
#include <sys/msg.h>
#include <cstring>

struct Msg {
    long type;
    char text[256];
};

int main()
{
    const long key = 12345;
    const std::string message = "Hello MsgQueue";

    int qid = msgget(key, 0666 | IPC_CREAT);
    if (qid < 0) {
        std::cerr << "msgget failed" << std::endl;
        return 1;
    }

    Msg msg{};
    msg.type = 1;
    strncpy(msg.text, message.data(), message.size());

    if (msgsnd(qid, &msg, sizeof(msg.text), 0) < 0) {
        std::cerr << "msgsnd failed" << std::endl;
        return 1;
    }

    std::cout << "send: " << message << std::endl;
    return 0;
}
