# 子目录通用编译规则，通过 make -C <dir> 调用
# 每个子目录需要定义 BIN 变量（如 BIN := publisher subscriber）

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall

AERON_DIR   := $(abspath $(CURDIR)/../aeron)
BUILD_DIR   := $(AERON_DIR)/cppbuild/Release
INCLUDES    := -I$(AERON_DIR)/aeron-client/src/main/cpp_wrapper \
               -I$(AERON_DIR)/aeron-client/src/main/c
AERON_LIB   := $(BUILD_DIR)/lib/libaeron_static.a
LDFLAGS     := -lpthread

all: $(BIN)

%: %.cpp $(AERON_LIB)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< $(AERON_LIB) $(LDFLAGS) -o $@

# 首次构建 aeron 库（需要 cmake，约几分钟）
$(AERON_LIB):
	cmake -S $(AERON_DIR) -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release
	cmake --build $(BUILD_DIR) --target aeron_static -j$$(nproc)

clean:
	rm -f $(BIN)

.PHONY: all clean
