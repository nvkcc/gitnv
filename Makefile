BUILD_DIR := target

MAKEFILE_PATH := $(realpath $(lastword $(MAKEFILE_LIST)))
MAKEFILE_DIR := $(realpath $(dir $(MAKEFILE_PATH)))

VALGRIND_LOG := $(MAKEFILE_DIR)/valgrind-log-%p.txt

VALGRIND_FLAGS :=
VALGRIND_FLAGS += --trace-children=yes
VALGRIND_FLAGS += --show-error-list=yes
VALGRIND_FLAGS += --leak-check=full
# VALGRIND_FLAGS += --show-leak-kinds=all
VALGRIND_FLAGS += --log-file=$(VALGRIND_LOG)
# VALGRIND_FLAGS += --xml=yes
# VALGRIND_FLAGS += --xml-file=valgrind.xml

# Either release or debug.
RELEASE := false

ifeq ($(RELEASE), true)
BUILD_TYPE := Release
else
BUILD_TYPE := Debug
endif

TEST_DIR := ~/repos
TEST_DIR := ~/repos/Algebra/Algebra/DummitFoote

current: test_add
# current: test_status
# current: run
# current: v

configure:
	cmake -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -S . -B $(BUILD_DIR) -G Ninja

build:
	cmake --build $(BUILD_DIR) --parallel 4

test_bufreader: install
	cat /home/khang/repos/Algebra/Algebra/DummitFoote/S02_E03_Cyclic_Groups_and_Subgroups.lean | git-nv

test_status: install
	git -C ~/repos/alatty/kittens/ask nv status

test_add: install
	git -C ~/repos/alatty/kittens/ask nv add thisfileshouldnotexist

run: install
	# ================================================================
	git -C ~/repos/Algebra/Algebra/DummitFoote nv status
	# ================================================================
	git -C ~/repos/Algebra/Algebra/DummitFoote status

install: build
	cmake --install $(BUILD_DIR)

v: install
	-@rm -f valgrind-log*.txt
	-cd $(TEST_DIR) && \
		valgrind $(VALGRIND_FLAGS) -- git-nv status
	-nvim valgrind-log*

test: build
	$(BUILD_DIR)/git-nv-test

fmt:
	git ls-files '*.c' '*.h' | xargs clang-format -i
