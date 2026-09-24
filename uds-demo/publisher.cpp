#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int main()
{
    const std::string sock_path = "/tmp/ipc_demo.sock";
    const std::string message = "Hello UDS";

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "socket failed" << std::endl;
        return 1;
    }

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sock_path.c_str(), sizeof(addr.sun_path) - 1);

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
