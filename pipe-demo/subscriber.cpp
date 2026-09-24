#include <iostream>
#include <string>
#include <unistd.h>

int main()
{
    // 从 stdin 读取，配合管道使用: ./publisher | ./subscriber
    std::string message;
    std::getline(std::cin, message);
    if (!message.empty()) {
        std::cout << "recv: " << message << std::endl;
    }
    return 0;
}
