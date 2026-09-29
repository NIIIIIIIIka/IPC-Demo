# aeron-demo

基于 [Aeron](https://github.com/aeron-io/aeron) 的 C++ IPC 发布/订阅演示。所有示例使用 `aeron:ipc`，通过本机共享内存在同一台机器的进程之间传递消息。

## 示例目录

| 目录 | 演示内容 |
|------|----------|
| `basic/` | 一条消息的基本发布订阅流程 |
| `streaming/` | 可配置消息数、消息大小、发送速率和慢消费者；输出吞吐及发布端背压计数 |
| `large-message/` | 大消息分片、`FragmentAssembler` 重组和校验 |
| `exclusive/` | `ExclusivePublication`、`tryClaim` 及两个 Stream 并发发送 |

## 依赖和构建

- CMake >= 3.6
- 支持 C++17 的编译器（g++ / clang++）
- `aeron/` 目录包含 Aeron 源码（git submodule 或直接拷贝）

在任意子目录构建单个示例，或从本目录构建全部示例：

```bash
cd streaming && make
# 或在 aeron-demo 根目录：
make
```

首次构建会编译 Aeron 静态库，需要几分钟。

## 启动

先启动 Media Driver：

```bash
./aeron/cppbuild/Release/binaries/aeronmd
```

然后在另一个终端启动订阅者，再启动发布者。下面命令假设当前目录是 `aeron-demo/`：

```bash
cd streaming
./subscriber 0 fast
./publisher 500000 256 0
```

发布者参数依次为 `消息数 消息字节数 目标消息/秒`。默认值为 `1000000 256 0`；速率设为 `0` 表示尽可能快地发送。消息长度限制为 1 字节到 1 MiB。订阅者参数为 `每条消息后的延迟微秒 标签`，默认为 `0 consumer`。

## 演示场景

### 1. 不同消息大小

保持订阅端不变，分别运行：

```bash
./publisher 500000 64 0
./publisher 500000 1024 0
./publisher 500000 65536 0
```

比较发布者和订阅者打印的消息吞吐（msg/s）与字节吞吐（MiB/s）。每轮需要重新运行有限条数的 publisher。`large-message/` 另展示大消息跨 fragment 发送后如何重组。

### 2. 不同发布频率

固定消息大小和消息数，只改变目标速率：

```bash
./publisher 500000 256 10000
./publisher 500000 256 100000
./publisher 500000 256 0
```

目标速率是发送节奏上限，不是保证达到的速率；订阅端处理能力或 CPU 不足时，实际吞吐会更低。

### 3. 多个订阅者

在多个终端启动订阅者，每个订阅者使用相同的 `aeron:ipc` 和 Stream ID `2001`：

```bash
./subscriber 0 consumer-A
./subscriber 0 consumer-B
./subscriber 0 consumer-C
```

再启动 publisher。Aeron 会向每个订阅该 Stream 的订阅者分别传送消息；每个终端独立打印接收速率。订阅者数量增加时，总体资源消耗也会增加。

### 4. 慢消费者和背压

让订阅端每条消息处理后暂停 100 微秒：

```bash
./subscriber 100 slow
./publisher 500000 256 0
```

观察发布端 `back-pressure` 计数和订阅端接收速率。背压表示发布端当前无法继续推进；示例会重试，直到发送完指定数量。它不是丢包计数。IPC 的发布位置会受到订阅者进度约束，慢订阅者可能降低发布端推进速度。

也可以同时运行一个快消费者和一个慢消费者，观察各自接收速率及慢消费者对共享流发布进度的影响。

### 5. CPU 竞争

可在 Linux 上用 `stress-ng` 制造可控 CPU 竞争（先安装该工具）：

```bash
stress-ng --cpu 0 --timeout 30s
```

`--cpu 0` 会按可用 CPU 数启动压力 worker，适合观察接近满载时的表现；要比较不同竞争程度，可改成 `--cpu 1` 或 `--cpu 2`。在压力运行期间执行同一组 publisher/subscriber 参数，记录吞吐和运行环境。实际占用还受容器配额和其他负载影响；应记录处理器型号、核心数、后台负载和 Aeron 参数，避免把不同机器的结果直接比较。

### 6. 跨 NUMA 节点（Linux 进阶）

需要多 NUMA 节点主机和 `numactl`。先查看拓扑：

```bash
numactl --hardware
```

让 Media Driver 绑定到 NUMA 节点 0，客户端绑定到节点 1（根据机器实际节点和 CPU 编号调整）：

```bash
numactl --cpunodebind=0 --membind=0 ./aeron/cppbuild/Release/binaries/aeronmd
numactl --cpunodebind=1 --membind=1 ./subscriber 0 remote-node
numactl --cpunodebind=1 --membind=1 ./publisher 500000 256 0
```

比较同节点绑定和跨节点绑定结果。Aeron IPC 的共享内存仍位于单机上；NUMA 场景测量的是跨内存节点访问的影响，不是网络通信。`membind` 可能因权限或可用内存不足而失败。

### 7. 进程退出与重启

保持 Media Driver 运行。启动订阅者和 publisher，发送完成后再次启动 publisher，观察仍运行的订阅者是否继续接收新进程发布的数据。再尝试重启订阅者并运行新一轮有限消息发送。

进程重启后 Aeron 客户端会重新注册发布或订阅，但这不等于持久化恢复：IPC 不会自动保存进程退出期间的消息。新订阅者通常从注册时的流位置开始接收；需要断线期间可重放或不丢消息时，应另行设计持久化/Archive 和应用层确认机制。若 Media Driver 本身退出，所有客户端都需要重新连接，且共享目录中的陈旧状态可能需要清理后才能重启。

## 其他示例

```bash
cd basic && ./subscriber       # 终端 1
cd basic && ./publisher        # 终端 2

cd large-message && ./subscriber
cd large-message && ./publisher

cd exclusive && ./subscriber
cd exclusive && ./publisher
```

## 关键参数

| 参数 | 值 | 说明 |
|------|----|------|
| Channel | `aeron:ipc` | 本机 IPC，不经过网络 |
| Stream ID | `1001` / `2001` / `3001,3002` | 区分数据流；同一组发布者和订阅者必须一致 |

## 清理

```bash
make clean
rm -rf aeron/cppbuild
```
