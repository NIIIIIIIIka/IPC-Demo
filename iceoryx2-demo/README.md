# iceoryx2-demo

基于 [Eclipse iceoryx2](https://github.com/eclipse-iceoryx/iceoryx2) C++ API 的单机跨进程通信示例。两个示例都使用 `ServiceType::Ipc`，不需要类似 RouDi 的中心守护进程。

## 示例

| 目录 | 说明 |
|---|---|
| `basic/` | 固定大小结构体的发布/订阅，展示 `loan_uninit()`、原地构造和 `send()` |
| `large-message/` | 10 条 1 MiB 动态 payload，展示共享内存 slice、零拷贝接收和内容校验 |
| `payload-benchmark/` | 依次比较 64 B 与 1 MiB；分开统计原地填充和 `send()`，自带 CMake 与一键启动脚本 |

> 注意：不同进程可以把同一共享内存映射到不同虚拟地址。日志中的地址只用于观察映射，不能单独证明两个地址对应同一物理页；零拷贝依据是 iceoryx2 的 loaned sample 生命周期和收发路径中没有 payload `memcpy`。

## 依赖

- Linux x86_64 或 aarch64
- CMake 3.22+
- 支持 C++17 的编译器
- Rust 工具链和 Cargo
- iceoryx2 构建依赖，Ubuntu 可参考官方脚本 `internal/scripts/install_dependencies_ubuntu.sh`

目录默认期望 iceoryx2 源码位于 `iceoryx2-demo/iceoryx2/`：

```bash
cd IPC-Demo/iceoryx2-demo
git clone --depth 1 https://github.com/eclipse-iceoryx/iceoryx2.git
```

也可以通过 `ICEORYX2_DIR=/path/to/iceoryx2 make` 使用已有源码目录。

## 构建

```bash
cd IPC-Demo/iceoryx2-demo
make
```

首次构建会编译并安装 iceoryx2 C/C++ bindings 到源码树下的 `target/ff/cc/install`，耗时通常明显长于其他 IPC Demo。

## 一键运行 Payload 对比（推荐）

Linux 下执行：

```bash
cd IPC-Demo/iceoryx2-demo/payload-benchmark
./run_demo.sh
```

脚本会在需要时下载、构建并安装 iceoryx2，然后自动完成 64 B 和 1 MiB 两轮 subscriber/publisher 演示。每轮默认 200 条，可通过 `SMALL_COUNT`、`LARGE_COUNT` 调整：

```bash
SMALL_COUNT=500 LARGE_COUNT=100 ./run_demo.sh
```

输出把“原地填充 payload”和“提交共享内存句柄”的时间分开统计。1 MiB 的初始化成本必然高于 64 B；零拷贝保证的是 `send/receive` 阶段不再复制整个 payload，而不是生成 1 MiB 数据不需要时间。

手工 CMake 构建：

```bash
cmake -S payload-benchmark -B payload-benchmark/build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$PWD/iceoryx2/target/ff/cc/install"
cmake --build payload-benchmark/build -j"$(nproc)"
```

## 运行 basic

先启动 subscriber：

```bash
./basic/subscriber
```

再在另一个终端启动 publisher：

```bash
./basic/publisher
```

发布方直接在 loaned sample 中构造 `TransmissionData`，订阅方获得只读 sample view。

## 运行 large-message

先启动 subscriber：

```bash
./large-message/subscriber
```

再在另一个终端启动 publisher：

```bash
./large-message/publisher
```

发布方从共享内存池借出 1 MiB slice 并直接填充；订阅方不复制 payload，直接读取并校验 1 MiB 共享样本。

## 清理

```bash
make clean       # 只删除本 Demo 的构建目录和可执行文件
make distclean   # 另外删除 iceoryx2 构建与安装产物
```

## API 参考

- <https://github.com/eclipse-iceoryx/iceoryx2/tree/main/examples/cxx/publish_subscribe>
- <https://github.com/eclipse-iceoryx/iceoryx2/tree/main/examples/cxx/publish_subscribe_dynamic_data>
- <https://github.com/eclipse-iceoryx/iceoryx2/tree/main/iceoryx2-cxx>
