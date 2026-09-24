#include <iostream>
#include <string>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <atomic>
#include <chrono>
#include <thread>

struct SharedMsg {
    std::atomic<bool> ready{false};
    char data[256];
};

int main()
{
    const char* shm_name = "/ipc_demo_shm";

    std::cout << "Waiting for publisher..." << std::endl;

    // 轮询等待共享内存段创建
    int fd = -1;
    while (fd < 0) {
        fd = shm_open(shm_name, O_RDWR, 0666);
        if (fd < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    auto* shm = static_cast<SharedMsg*>(
        mmap(nullptr, sizeof(SharedMsg), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));

    // 轮询等待消息就绪
    while (!shm->ready.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::string message(shm->data);
    std::cout << "recv: " << message << std::endl;

    shm->ready.store(false);

    // 由 subscriber 负责清理
    munmap(shm, sizeof(SharedMsg));
    close(fd);
    shm_unlink(shm_name);
    return 0;
}
