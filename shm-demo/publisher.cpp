#include <iostream>
#include <string>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <atomic>
#include <cstring>

struct SharedMsg {
    std::atomic<bool> ready{false};
    char data[256];
};

int main()
{
    const char* shm_name = "/ipc_demo_shm";
    const std::string message = "Hello SHM";

    // 创建并初始化共享内存段
    int fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (fd < 0) {
        std::cerr << "shm_open failed" << std::endl;
        return 1;
    }

    int fr = ftruncate(fd, sizeof(SharedMsg));
    (void)fr;

    auto* shm = static_cast<SharedMsg*>(
        mmap(nullptr, sizeof(SharedMsg), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));

    strncpy(shm->data, message.data(), message.size());
    shm->data[message.size()] = '\0';
    shm->ready.store(true);

    std::cout << "send: " << message << std::endl;

    // 不清理，由 subscriber 负责
    munmap(shm, sizeof(SharedMsg));
    close(fd);
    return 0;
}
