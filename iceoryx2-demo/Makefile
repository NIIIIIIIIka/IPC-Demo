ICEORYX2_DIR ?= $(abspath iceoryx2)
ICEORYX2_BUILD_DIR ?= $(ICEORYX2_DIR)/target/ff/cc/build
ICEORYX2_INSTALL_DIR ?= $(ICEORYX2_DIR)/target/ff/cc/install
BUILD_DIR ?= build

all: configure
	cmake --build $(BUILD_DIR) -j$$(nproc)

configure: $(ICEORYX2_INSTALL_DIR)/lib/cmake/iceoryx2-cxx/iceoryx2-cxxConfig.cmake
	cmake -S . -B $(BUILD_DIR) \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_PREFIX_PATH=$(ICEORYX2_INSTALL_DIR)

$(ICEORYX2_INSTALL_DIR)/lib/cmake/iceoryx2-cxx/iceoryx2-cxxConfig.cmake:
	@test -f "$(ICEORYX2_DIR)/CMakeLists.txt" || \
		(printf '%s\n' \
		 'iceoryx2 source not found: $(ICEORYX2_DIR)' \
		 'Clone it with:' \
		 '  git clone --depth 1 https://github.com/eclipse-iceoryx/iceoryx2.git $(ICEORYX2_DIR)' && false)
	cmake -S $(ICEORYX2_DIR) -B $(ICEORYX2_BUILD_DIR) \
		-DCMAKE_BUILD_TYPE=Release \
		-DBUILD_EXAMPLES=OFF
	cmake --build $(ICEORYX2_BUILD_DIR) -j$$(nproc)
	cmake --install $(ICEORYX2_BUILD_DIR) --prefix $(ICEORYX2_INSTALL_DIR)

clean:
	rm -rf $(BUILD_DIR)
	rm -f basic/publisher basic/subscriber
	rm -f large-message/publisher large-message/subscriber

distclean: clean
	rm -rf $(ICEORYX2_BUILD_DIR) $(ICEORYX2_INSTALL_DIR)

.PHONY: all configure clean distclean
