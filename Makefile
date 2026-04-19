BUILD_DIR := target

MAKEFILE_PATH := $(realpath $(lastword $(MAKEFILE_LIST)))
MAKEFILE_DIR := $(dir $(MAKEFILE_PATH))

VALGRIND_LOG := $(MAKEFILE_DIR)/valgrind-log-%p.txt

VALGRIND_FLAGS :=
VALGRIND_FLAGS += --trace-children=yes
VALGRIND_FLAGS += --show-error-list=yes
# VALGRIND_FLAGS += --leak-check=full
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

# current: run
current: v

configure:
	cmake -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -S . -B $(BUILD_DIR) -G Ninja

build:
	cmake --build $(BUILD_DIR) --parallel 4

run: install
	git -C ~/repos/Algebra/Algebra/DummitFoote nv status

install: build
	cmake --install $(BUILD_DIR)

v: install
	-@rm -f valgrind-log*.txt
	-cd ~/repos/Algebra/Algebra/DummitFoote && \
		valgrind $(VALGRIND_FLAGS) -- git-nv status

test: configure install
	cargo test -- --test-threads=1

fmt:
	git ls-files '*.c' '*.h' | xargs clang-format -i
