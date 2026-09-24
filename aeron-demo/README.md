# aeron-demo

基于 [Aeron](https://github.com/aeron-io/aeron) 的 IPC 发布/订阅演示，使用 `aeron:ipc` 通道进行本机进程间高性能消息传递。

## 依赖

- CMake >= 3.6
- 支持 C++17 的编译器（g++ / clang++）
- `aeron/` 目录为 Aeron 源码（git submodule 或直接拷贝），首次构建会自动编译出 `libaeron_static.a`

## 目录结构

每个子目录是一个独立示例，包含 `publisher.cpp` + `subscriber.cpp` + `Makefile`：

| 目录 | 说明 |
|------|------|
| `basic/` | 单条消息发布/订阅，阻塞等待连接 |
| `streaming/` | 高吞吐流：持续发送百万级消息，处理背压并统计吞吐量 |
| `large-message/` | 大消息分片：1 MiB 消息由 Aeron 自动分片，`FragmentAssembler` 重组并校验 |
| `exclusive/` | 独占发布：`ExclusivePublication` + `tryClaim` 零拷贝，单进程多 stream 交错发送 |

## 构建

在任意子目录单独构建：

```bash
cd basic && make        # 或 streaming / large-message / exclusive
```

或在 `aeron-demo/` 根目录一次构建全部示例：

```bash
make
```

首次运行会自动执行以下步骤（需要几分钟）：

```bash
cmake -S aeron -B aeron/cppbuild/Release -DCMAKE_BUILD_TYPE=Release
cmake --build aeron/cppbuild/Release --target aeron_static -j$(nproc)
```

之后再执行 `make` 只会编译各目录下的 `*.cpp`，速度很快。

## 运行

所有示例都需要先启动 Aeron media driver（client 本身不含驱动）：

```bash
./aeron/cppbuild/Release/binaries/aeronmd &
```

然后开 **两个终端**，先启动 subscriber，再启动 publisher：

```bash
# 终端 1
cd basic && ./subscriber
# 终端 2
cd basic && ./publisher
```

### 高吞吐流

```bash
cd streaming
./subscriber                 # 终端 1
./publisher 500000           # 终端 2，默认 1,000,000 条，可参数指定
```

### 大消息分片

```bash
cd large-message
./subscriber                 # 终端 1
./publisher                  # 终端 2，发送 10 条 1 MiB 消息
```

### 独占发布

```bash
cd exclusive
./subscriber                 # 终端 1
./publisher                  # 终端 2，stream 3001 / 3002 各发 1000 条
```

## 关键参数

| 参数 | 值 | 说明 |
|------|----|------|
| channel | `aeron:ipc` | 本机 IPC，不走网络 |
| streamId | 1001 / 2001 / 2002 / 3001,3002 | 对应 basic / streaming / large-message / exclusive，发布与订阅必须一致 |

## 清理

```bash
make clean        # 清理所有子目录的二进制
rm -rf aeron/cppbuild  # （可选）删除 aeron 库构建产物
```
