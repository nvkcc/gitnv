$(BUILD_DIR)/.cmake-commands---:
	@-rm -f $(BUILD_DIR)/.cmake-commands-*
	@touch CMakeLists.txt $(BUILD_DIR)/.cmake-commands---
---record: $(BUILD_DIR)/.cmake-commands---
$(BUILD_DIR)/.cmake-commands-build:
	@-rm -f $(BUILD_DIR)/.cmake-commands-*
	@touch CMakeLists.txt $(BUILD_DIR)/.cmake-commands-build
build-record: $(BUILD_DIR)/.cmake-commands-build
$(BUILD_DIR)/.cmake-commands-test:
	@-rm -f $(BUILD_DIR)/.cmake-commands-*
	@touch CMakeLists.txt $(BUILD_DIR)/.cmake-commands-test
test-record: $(BUILD_DIR)/.cmake-commands-test
