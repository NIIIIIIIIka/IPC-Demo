#include <iostream>
#include <string>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

int main()
{
    const int port = 9999;
    const std::string message = "Hello TCP";

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "socket failed" << std::endl;
        return 1;
    }

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        std::cerr << "connect failed, is subscriber running?" << std::endl;
        return 1;
    }

    ssize_t wr = write(fd, message.data(), message.size());
    (void)wr;
    close(fd);

    std::cout << "send: " << message << std::endl;
    return 0;
}
