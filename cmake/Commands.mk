$(BUILD_DIR)/.cmake-commands-build:
	@-rm -f $(BUILD_DIR)/.cmake-commands-*
	@mkdir -p $(BUILD_DIR)
	@touch CMakeLists.txt $(BUILD_DIR)/.cmake-commands-build
build-record: $(BUILD_DIR)/.cmake-commands-build
$(BUILD_DIR)/.cmake-commands-test:
	@-rm -f $(BUILD_DIR)/.cmake-commands-*
	@mkdir -p $(BUILD_DIR)
	@touch CMakeLists.txt $(BUILD_DIR)/.cmake-commands-test
test-record: $(BUILD_DIR)/.cmake-commands-test
