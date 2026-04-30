BUILD_DIR ?= build
CMAKE ?= cmake
CMAKE_ARGS ?=
CMAKE_BUILD_ARGS ?=
CMAKE_INSTALL_ARGS ?= -DINSTALL_DOCS=ON -DINSTALL_MANFILES=ON

.PHONY: all help build-docs build-docs-pdf install-docs clean configure

help:
	@echo "Available targets: build-docs, build-docs-pdf, install-docs, clean"

all: build-docs

configure:
	$(CMAKE) -S . -B $(BUILD_DIR) $(CMAKE_ARGS)

build-docs: configure
	$(CMAKE) --build $(BUILD_DIR) --target wirble-docs $(CMAKE_BUILD_ARGS)

build-docs-pdf: configure
	$(CMAKE) --build $(BUILD_DIR) --target wirble-docs-pdf $(CMAKE_BUILD_ARGS)

install-docs: configure
	$(CMAKE) -S . -B $(BUILD_DIR) $(CMAKE_ARGS) $(CMAKE_INSTALL_ARGS)
	$(CMAKE) --build $(BUILD_DIR) --target install $(CMAKE_BUILD_ARGS)

clean:
	$(RM) -rf $(BUILD_DIR)
