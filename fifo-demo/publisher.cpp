#include <iostream>
#include <string>
#include <fcntl.h>
#include <unistd.h>

int main()
{
    const std::string fifo_path = "/tmp/ipc_demo_fifo";
    const std::string message = "Hello FIFO";

    // O_WRONLY 会在无读者时阻塞，确保 subscriber 先启动
    int fd = open(fifo_path.c_str(), O_WRONLY);
    if (fd < 0) {
        std::cerr << "open fifo failed" << std::endl;
        return 1;
    }

    ssize_t wr = write(fd, message.data(), message.size());
    (void)wr;
    close(fd);

    std::cout << "send: " << message << std::endl;
    return 0;
}
