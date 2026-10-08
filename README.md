# IPC Demos

多种 Linux 进程间通信（IPC）方式的演示样例，风格参考 `aeron-demo`。

## 目录结构

| 目录 | 通信方式 | 说明 |
|------|----------|------|
| `aeron-demo/` | Aeron (UDP IPC) | 基于 Aeron 库的高性能 pub/sub，使用 `aeron:ipc` 通道 |
| `iceoryx2-demo/` | iceoryx2 (零拷贝共享内存) | C++ Pub/Sub，展示固定结构体和 1 MiB 动态 payload 的 loan/send/receive |
| `pipe-demo/` | 匿名管道 (Pipe) | 最简单的进程间通信，通过 shell 管道 `|` 串联 |
| `fifo-demo/` | 命名管道 (FIFO) | 通过文件系统中的命名管道传输数据，独立进程间通信 |
| `uds-demo/` | Unix Domain Socket | 基于 `AF_UNIX` 的本地 socket 通信，支持 stream/datagram |
| `tcp-demo/` | TCP Socket (localhost) | 基于回环地址的 TCP 通信，与远程 TCP 协议一致 |
| `shm-demo/` | 共享内存 (POSIX SHM) | 基于 `shm_open` + `mmap` 的共享内存，配合原子标志位同步 |
| `msgqueue-demo/` | System V 消息队列 | 基于 `msgget`/`msgsnd`/`msgrcv` 的队列式通信 |

## 构建

每个 demo 目录都有独立的 `Makefile`：

```bash
# 编译单个 demo
cd pipe-demo && make

# 编译所有 demo
make -C pipe-demo && make -C fifo-demo && make -C uds-demo \
  && make -C tcp-demo && make -C shm-demo && make -C msgqueue-demo

# 编译需要额外依赖的 Aeron 与 iceoryx2 示例
make optional
```

## 运行方式

每个 demo 都是 `publisher`（发送端）+ `subscriber`（接收端）两个独立程序。

### pipe-demo — 匿名管道

```bash
cd pipe-demo
./publisher | ./subscriber
```

一条 shell 命令即可完成，最基础的 IPC。

### fifo-demo — 命名管道

```bash
cd fifo-demo
# 终端 1：先启动 subscriber（创建并等待）
./subscriber
# 终端 2：再启动 publisher
./publisher
```

注意：`O_WRONLY` 打开 FIFO 时会阻塞直到有读者，所以 subscriber 要先运行。

### uds-demo — Unix Domain Socket

```bash
cd uds-demo
# 终端 1
./subscriber
# 终端 2
./publisher
```

socket 文件位于 `/tmp/ipc_demo.sock`，仅限本机通信。

### tcp-demo — TCP Socket

```bash
cd tcp-demo
# 终端 1
./subscriber
# 终端 2
./publisher
```

监听 `127.0.0.1:9999`，走回环接口。

### shm-demo — 共享内存

```bash
cd shm-demo
# 终端 1
./subscriber
# 终端 2
./publisher
```

通过 `shm_open` 创建 POSIX 共享内存段，使用 `std::atomic<bool>` 作为同步标志。

### msgqueue-demo — System V 消息队列

```bash
cd msgqueue-demo
# 先启动 publisher 创建队列
./publisher
# 再启动 subscriber 接收
./subscriber
```

队列 key 为 `12345`，subscriber 接收后自动清理队列。

### iceoryx2-demo — 零拷贝共享内存

包含固定结构体和 1 MiB 动态 payload 两个示例。详细的依赖、构建和运行步骤见 [`iceoryx2-demo/README.md`](iceoryx2-demo/README.md)。

```bash
cd iceoryx2-demo
git clone --depth 1 https://github.com/eclipse-iceoryx/iceoryx2.git
make

# 终端 1
./large-message/subscriber
# 终端 2
./large-message/publisher
```

### 新增的一键演示

以下脚本会自动执行 CMake 配置、构建并拉起所需进程（仅支持 Linux）：

```bash
# Aeron 显式背压
cd aeron-demo/backpressure && ./run_demo.sh

# iceoryx2：64 B 与 1 MiB 零拷贝路径对比
cd iceoryx2-demo/payload-benchmark && ./run_demo.sh
```

## 各方式对比

| 方式 | 方向 | 生命周期 | 同步机制 | 典型场景 |
|------|------|----------|----------|----------|
| Pipe | 单向 | 进程结束即销毁 | 内核缓冲 | 父进程→子进程 |
| FIFO | 单向 | 文件系统对象，可独立于进程 | open 阻塞 | 独立进程间 |
| UDS | 双向 | socket 文件 | 标准 socket API | 同机服务间 |
| TCP | 双向 | 连接生命周期 | 标准 socket API | 跨机通信 |
| SHM | 双向 | 显式清理 | 需自行实现（信号量/原子变量） | 高性能数据交换 |
| MsgQueue | 单向 | 显式清理 | 内核队列 | 异步消息传递 |
