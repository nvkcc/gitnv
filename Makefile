MAKEFILE_PATH := $(realpath $(lastword $(MAKEFILE_LIST)))
MAKEFILE_DIR := $(realpath $(dir $(MAKEFILE_PATH)))

RED := $(shell echo "\033[31m")
YELLOW := $(shell echo "\033[33m")
END := $(shell echo "\033[m")

BUILD_DIR := $(MAKEFILE_DIR)/target

VALGRIND_LOG := $(MAKEFILE_DIR)/valgrind-log-%p.txt

VALGRIND_FLAGS :=
VALGRIND_FLAGS += --trace-children=yes
VALGRIND_FLAGS += --show-error-list=yes
VALGRIND_FLAGS += --leak-check=full
VALGRIND_FLAGS += --show-leak-kinds=all
VALGRIND_FLAGS += --log-file=$(VALGRIND_LOG)
# VALGRIND_FLAGS += --xml=yes
# VALGRIND_FLAGS += --xml-file=valgrind.xml

# One of: Debug | Release | RelWithDebInfo | MinSizeRel
CMAKE_BUILD_TYPE := Release

# One of: ON | OFF
BUILD_GITNV_TESTS := OFF

DEV_DIR := ~/repos
DEV_DIR := ~/repos/alatty/kittens/ask

COMMAND_FILE := $(MAKEFILE_DIR)/cmake/Commands.mk

current: setup-commands
current: test

$(COMMAND_FILE): cmake/setup_commands.py
	@python3 cmake/setup_commands.py \
		build \
		test \
		> $(COMMAND_FILE)
setup-commands: $(COMMAND_FILE)
-include $(COMMAND_FILE)

# Quick hack to externally enforce that CMake re-runs the configuration upon
# changes to CMakeLists.txt
$(BUILD_DIR)/CMakeFiles: CMakeLists.txt
	@echo "\e[33m-------- CONFIGURE CMAKE --------\e[m"
	cmake -G Ninja -S . \
		-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
		-DBUILD_GITNV_TESTS=$(BUILD_GITNV_TESTS) \
		-B $(BUILD_DIR)

configure: $(BUILD_DIR)/CMakeFiles
# End of the quick hack.

remove-cmake-binary-dir:
	rm -rf $(BUILD_DIR)

reconfigure: remove-cmake-binary-dir configure

build: build-record configure
	cmake --build $(BUILD_DIR) --parallel 4

install: build
	cmake --install $(BUILD_DIR)

test: CMAKE_BUILD_TYPE := Debug
test: BUILD_GITNV_TESTS := ON
test: test-record configure
	cmake --build $(BUILD_DIR) --parallel 4
	# cd $(BUILD_DIR) && ctest # run CTest. That's one way of doing things.
	$(BUILD_DIR)/git-nv-test

v: install
	-@rm -f $(MAKEFILE_DIR)/valgrind-log*.txt
	-cd $(DEV_DIR) && valgrind $(VALGRIND_FLAGS) -- git-nv status

fmt:
	git ls-files '*.c' '*.h' | xargs clang-format -i
