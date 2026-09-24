#include <iostream>
#include <string>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

int main()
{
    const int port = 9999;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "socket failed" << std::endl;
        return 1;
    }

    // 允许端口复用，避免 TIME_WAIT 阻塞
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        std::cerr << "bind failed" << std::endl;
        return 1;
    }

    listen(fd, 1);
    std::cout << "Listening on 127.0.0.1:" << port << std::endl;

    int client = accept(fd, nullptr, nullptr);
    if (client < 0) {
        std::cerr << "accept failed" << std::endl;
        return 1;
    }

    char buf[256] = {};
    ssize_t n = read(client, buf, sizeof(buf) - 1);
    if (n > 0) {
        std::string message(buf, n);
        std::cout << "recv: " << message << std::endl;
    }

    close(client);
    close(fd);
    return 0;
}
