#include <iostream>
#include <string>
#include <unistd.h>

int main()
{
    // 通过 stdout 发送，配合管道使用: ./publisher | ./subscriber
    const std::string message = "Hello Pipe";
    std::cout << message << std::endl;
    return 0;
}
