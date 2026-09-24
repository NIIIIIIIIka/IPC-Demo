#include <iostream>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

int main()
{
    const std::string fifo_path = "/tmp/ipc_demo_fifo";
    const char* fifo = fifo_path.c_str();

    // 创建 FIFO，若不存在
    if (access(fifo, F_OK) != 0) {
        int r = mkfifo(fifo, 0666);
        (void)r;
    }

    // O_RDWR 避免打开时阻塞
    int fd = open(fifo, O_RDWR);
    if (fd < 0) {
        std::cerr << "open fifo failed" << std::endl;
        return 1;
    }

    std::cout << "Waiting for publisher..." << std::endl;

    char buf[256] = {};
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    if (n > 0) {
        std::string message(buf, n);
        std::cout << "recv: " << message << std::endl;
    }

    close(fd);
    unlink(fifo);
    return 0;
}
