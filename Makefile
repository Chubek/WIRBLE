BUILD_DIR ?= build
DOC_BUILD_DIR ?= docs/build
CMAKE ?= cmake
CMAKE_ARGS ?=
CMAKE_BUILD_ARGS ?=
INSTALL_ARGS ?=

.PHONY: all help build-wirble build-docs install-wirble install-docs clean

all: build-wirble

help:
	@echo "Available targets: build-wirble, build-docs, install-wirble, install-docs, clean"

build-wirble:
	$(CMAKE) -S . -B $(BUILD_DIR) $(CMAKE_ARGS)
	$(CMAKE) --build $(BUILD_DIR) $(CMAKE_BUILD_ARGS)

build-docs:
	$(MAKE) -C docs build-docs BUILD_DIR=$(DOC_BUILD_DIR)

install-wirble:
	$(MAKE) build-wirble
	$(CMAKE) --build $(BUILD_DIR) --target install -- $(INSTALL_ARGS)

install-docs:
	$(MAKE) build-docs
	$(MAKE) -C docs install-docs BUILD_DIR=$(DOC_BUILD_DIR)

clean:
	$(RM) -rf $(BUILD_DIR) $(DOC_BUILD_DIR)
