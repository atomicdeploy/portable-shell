# Compatibility entry point. CMake is the single source of build configuration.
CMAKE ?= cmake
BUILD_DIR ?= build
CMAKE_ARGS ?=
.PHONY: all configure examples test clean install
all: configure
	$(CMAKE) --build $(BUILD_DIR)
configure:
	$(CMAKE) -S . -B $(BUILD_DIR) -DPORTABLE_SHELL_BUILD_LEGACY=ON $(CMAKE_ARGS)
examples: all
test: all
	ctest --test-dir $(BUILD_DIR) --output-on-failure
clean:
	$(CMAKE) --build $(BUILD_DIR) --target clean
install: all
	$(CMAKE) --install $(BUILD_DIR)
