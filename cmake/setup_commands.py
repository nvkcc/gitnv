from sys import argv

for arg in argv[1:]:
    print(f"$(BUILD_DIR)/.cmake-commands-{arg}:")
    print(f"	@-rm -f $(BUILD_DIR)/.cmake-commands-*")
    print(f"	@mkdir -p $(BUILD_DIR)")
    print(f"	@touch CMakeLists.txt $(BUILD_DIR)/.cmake-commands-{arg}")
    print(f"{arg}-record: $(BUILD_DIR)/.cmake-commands-{arg}")
