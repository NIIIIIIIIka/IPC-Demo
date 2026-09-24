#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int main()
{
    const std::string sock_path = "/tmp/ipc_demo.sock";

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "socket failed" << std::endl;
        return 1;
    }

    // 清理残留 socket 文件
    unlink(sock_path.c_str());

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sock_path.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        std::cerr << "bind failed" << std::endl;
        return 1;
    }

    listen(fd, 1);
    std::cout << "Listening on " << sock_path << std::endl;

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
    unlink(sock_path.c_str());
    return 0;
}
