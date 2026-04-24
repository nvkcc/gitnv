MAKEFILE_PATH := $(realpath $(lastword $(MAKEFILE_LIST)))
MAKEFILE_DIR := $(realpath $(dir $(MAKEFILE_PATH)))

BUILD_DIR := $(MAKEFILE_DIR)/target

VALGRIND_LOG := $(MAKEFILE_DIR)/valgrind-log-%p.txt

VALGRIND_FLAGS :=
VALGRIND_FLAGS += --trace-children=yes
VALGRIND_FLAGS += --show-error-list=yes
VALGRIND_FLAGS += --leak-check=full
# VALGRIND_FLAGS += --show-leak-kinds=all
VALGRIND_FLAGS += --log-file=$(VALGRIND_LOG)
# VALGRIND_FLAGS += --xml=yes
# VALGRIND_FLAGS += --xml-file=valgrind.xml

# One of: Debug | Release | RelWithDebInfo | MinSizeRel
CMAKE_BUILD_TYPE := Debug

DEV_DIR := ~/repos
DEV_DIR := ~/repos/alatty/kittens/ask

current: goal

# Quick hack to externally enforce that CMake re-runs the configuration upon
# changes to CMakeLists.txt
$(BUILD_DIR)/CMakeFiles: CMakeLists.txt
	cmake -G Ninja -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE)

configure: $(BUILD_DIR)/CMakeFiles
# End of the quick hack.

# configure:
# 	cmake -G Ninja -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE)
#
# create-cmake-binary-dir:
# 	mkdir -p $(BUILD_DIR)
#
# remove-cmake-binary-dir:
# 	rm -rf $(BUILD_DIR)
#
# reconfigure: remove-cmake-binary-dir configure
#
# build:
# 	cmake --build $(BUILD_DIR) --parallel 4
#
# dev_status: install
# 	git -C $(DEV_DIR) nv status
#
# dev_add: install
# 	git -C $(DEV_DIR) nv add thisfileshouldnotexist 3 1 2..7
#
# install: build
# 	cmake --install $(BUILD_DIR)
#
# v: install
# 	-@rm -f $(MAKEFILE_DIR)/valgrind-log*.txt
# 	-cd $(DEV_DIR) && valgrind $(VALGRIND_FLAGS) -- git-nv status
#
# test: build
# 	$(BUILD_DIR)/git-nv-test
#
# fmt:
# 	git ls-files '*.c' '*.h' | xargs clang-format -i
