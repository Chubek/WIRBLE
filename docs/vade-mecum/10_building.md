# Chapter 9: Building and Using WIRBLE

## Overview

This chapter covers building WIRBLE from source, integrating it into your projects, and using the various build systems supported (CMake, Meson, Autotools).

## Build Systems

WIRBLE supports three build systems:
- **CMake**: Cross-platform, widely used
- **Meson**: Fast, modern build system
- **Autotools**: Traditional Unix build system

## Prerequisites

### Required Dependencies

- **C Compiler**: GCC 7+ or Clang 6+
- **Build System**: CMake 3.10+, Meson 0.50+, or Autotools
- **LMDB**: Lightning Memory-Mapped Database
- **Standard Libraries**: POSIX-compliant system

### Optional Dependencies

- **Doxygen**: For documentation generation
- **Python 3**: For build scripts and tools
- **Graphviz**: For visualization of IR graphs

## Building with CMake

### Basic Build

```bash
# Clone repository
git clone https://github.com/yourorg/wirble.git
cd wirble

# Create build directory
mkdir build
cd build

# Configure
cmake ..

# Build
cmake --build .

# Install (optional)
sudo cmake --install .
```

### Build Options

```bash
# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build with optimizations
cmake -DCMAKE_BUILD_TYPE=Release ..

# Enable all warnings
cmake -DWIRBLE_ENABLE_WARNINGS=ON ..

# Build with sanitizers
cmake -DWIRBLE_ENABLE_ASAN=ON ..
cmake -DWIRBLE_ENABLE_UBSAN=ON ..

# Build documentation
cmake -DWIRBLE_BUILD_DOCS=ON ..

# Build examples
cmake -DWIRBLE_BUILD_EXAMPLES=ON ..

# Build tests
cmake -DWIRBLE_BUILD_TESTS=ON ..
```

### CMake Configuration

Available CMake options:

```cmake
option(WIRBLE_BUILD_SHARED "Build shared libraries" ON)
option(WIRBLE_BUILD_STATIC "Build static libraries" ON)
option(WIRBLE_BUILD_EXAMPLES "Build examples" OFF)
option(WIRBLE_BUILD_TESTS "Build tests" OFF)
option(WIRBLE_BUILD_DOCS "Build documentation" OFF)
option(WIRBLE_ENABLE_WARNINGS "Enable compiler warnings" OFF)
option(WIRBLE_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(WIRBLE_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(WIRBLE_ENABLE_LTO "Enable Link-Time Optimization" OFF)
```

### Custom Installation Prefix

```bash
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
cmake --build .
sudo cmake --install .
```

## Building with Meson

### Basic Build

```bash
# Configure
meson setup build

# Build
meson compile -C build

# Install (optional)
sudo meson install -C build
```

### Build Options

```bash
# Debug build
meson setup build --buildtype=debug

# Release build
meson setup build --buildtype=release

# With specific options
meson setup build -Dbuild_examples=true -Dbuild_tests=true

# Reconfigure
meson configure build -Dbuild_docs=true
```

### Meson Configuration

Available Meson options (meson_options.txt):

```meson
option('build_examples', type: 'boolean', value: false)
option('build_tests', type: 'boolean', value: false)
option('build_docs', type: 'boolean', value: false)
option('enable_lto', type: 'boolean', value: false)
option('enable_asan', type: 'boolean', value: false)
```

## Building with Autotools

### Basic Build

```bash
# Generate configure script (if building from git)
./autogen.sh

# Configure
./configure

# Build
make

# Install (optional)
sudo make install
```

### Configuration Options

```bash
# Custom prefix
./configure --prefix=/usr/local

# Enable debug symbols
./configure --enable-debug

# Disable shared libraries
./configure --disable-shared

# Enable examples
./configure --enable-examples

# Enable tests
./configure --enable-tests
```

## Library Components

WIRBLE is organized into several libraries:

### Core Libraries

- **libwirble-common**: Common utilities (arena, strpool, diagnostics)
- **libwirble-wil**: WIL intermediate representation
- **libwirble-mal**: MAL intermediate representation
- **libwirble-wrs**: Rewrite system
- **libwirble-mds**: Machine description and instruction selection
- **libwirble-tos**: Target optimization system
- **libwirble-wvm**: Virtual machine
- **libwirble-support**: Support utilities

### Linking

When using WIRBLE, link against the components you need:

```bash
# Link all components
gcc mycompiler.c -lwirble-wil -lwirble-mal -lwirble-wrs -lwirble-mds -lwirble-tos -lwirble-common

# Or use pkg-config
gcc mycompiler.c $(pkg-config --cflags --libs wirble)
```

## Integration into Projects

### CMake Integration

```cmake
# Find WIRBLE
find_package(WIRBLE REQUIRED)

# Add executable
add_executable(mycompiler main.c)

# Link WIRBLE
target_link_libraries(mycompiler PRIVATE WIRBLE::wil WIRBLE::mal WIRBLE::common)
```

### pkg-config Integration

```bash
# Check if WIRBLE is installed
pkg-config --exists wirble

# Get compiler flags
CFLAGS=$(pkg-config --cflags wirble)

# Get linker flags
LDFLAGS=$(pkg-config --libs wirble)

# Compile
gcc $CFLAGS mycompiler.c $LDFLAGS -o mycompiler
```

### Manual Integration

```c
// Include headers
#include <wirble/wirble-wil.h>
#include <wirble/wirble-mal.h>
#include <wirble/wirble-common.h>

// Link libraries
// -lwirble-wil -lwirble-mal -lwirble-common -llmdb
```

## Header Files

WIRBLE provides modular headers:

```c
#include <wirble/wirble-wil.h>    // WIL API
#include <wirble/wirble-mal.h>    // MAL API
#include <wirble/wirble-wrs.h>    // WRS API
#include <wirble/wirble-mds.h>    // MDS API
#include <wirble/wirble-tos.h>    // TOS API
#include <wirble/wirble-wvm.h>    // WVM API
```

## Example Project Structure

```
mycompiler/
├── CMakeLists.txt
├── src/
│   ├── main.c
│   ├── parser.c
│   ├── codegen.c
│   └── optimizer.c
├── include/
│   └── mycompiler.h
└── tests/
    └── test_codegen.c
```

### CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.10)
project(mycompiler C)

# Find WIRBLE
find_package(WIRBLE REQUIRED)

# Add executable
add_executable(mycompiler
    src/main.c
    src/parser.c
    src/codegen.c
    src/optimizer.c
)

# Include directories
target_include_directories(mycompiler PRIVATE include)

# Link WIRBLE
target_link_libraries(mycompiler PRIVATE
    WIRBLE::wil
    WIRBLE::mal
    WIRBLE::wrs
    WIRBLE::mds
    WIRBLE::common
)

# C standard
set_property(TARGET mycompiler PROPERTY C_STANDARD 11)
```

## Running Tests

### CMake

```bash
# Build with tests
cmake -DWIRBLE_BUILD_TESTS=ON ..
cmake --build .

# Run tests
ctest

# Run with verbose output
ctest --verbose

# Run specific test
ctest -R test_wil
```

### Meson

```bash
# Build with tests
meson setup build -Dbuild_tests=true
meson compile -C build

# Run tests
meson test -C build

# Run with verbose output
meson test -C build --verbose
```

### Autotools

```bash
# Build with tests
./configure --enable-tests
make

# Run tests
make check

# Run specific test
./tests/test_wil
```

## Building Documentation

### Doxygen

```bash
# CMake
cmake -DWIRBLE_BUILD_DOCS=ON ..
cmake --build . --target docs

# Meson
meson setup build -Dbuild_docs=true
meson compile -C build docs

# Autotools
./configure --enable-docs
make docs

# View documentation
xdg-open build/docs/html/index.html
```

### Manual Doxygen

```bash
cd docs
doxygen Doxyfile
```

## Installation Locations

Default installation paths:

### Unix/Linux

```
/usr/local/
├── include/wirble/
│   ├── wirble-wil.h
│   ├── wirble-mal.h
│   └── ...
├── lib/
│   ├── libwirble-wil.so
│   ├── libwirble-mal.so
│   └── ...
└── share/
    ├── wirble/
    │   └── machines/
    │       ├── x86_64.json
    │       ├── aarch64.yaml
    │       └── wasm.xml
    └── doc/wirble/
```

### Windows

```
C:\Program Files\WIRBLE\
├── include\wirble\
├── lib\
└── share\
```

## Environment Variables

### WIRBLE_MACHINE_PATH

Path to machine description files:

```bash
export WIRBLE_MACHINE_PATH=/usr/local/share/wirble/machines:/home/user/custom-machines
```

### WIRBLE_CACHE_DIR

Directory for TOS cache database:

```bash
export WIRBLE_CACHE_DIR=/tmp/wirble-cache
```

## Cross-Compilation

### CMake Toolchain

```cmake
# toolchain-aarch64.cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

set(CMAKE_FIND_ROOT_PATH /usr/aarch64-linux-gnu)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
```

Build:

```bash
cmake -DCMAKE_TOOLCHAIN_FILE=toolchain-aarch64.cmake ..
cmake --build .
```

### Meson Cross-File

```ini
# cross-aarch64.txt
[binaries]
c = 'aarch64-linux-gnu-gcc'
cpp = 'aarch64-linux-gnu-g++'
ar = 'aarch64-linux-gnu-ar'
strip = 'aarch64-linux-gnu-strip'

[host_machine]
system = 'linux'
cpu_family = 'aarch64'
cpu = 'cortex-a53'
endian = 'little'
```

Build:

```bash
meson setup build --cross-file cross-aarch64.txt
meson compile -C build
```

## Troubleshooting

### Common Issues

#### LMDB Not Found

```bash
# Ubuntu/Debian
sudo apt-get install liblmdb-dev

# Fedora/RHEL
sudo dnf install lmdb-devel

# macOS
brew install lmdb
```

#### Missing Headers

Ensure WIRBLE is installed or set include path:

```bash
gcc -I/path/to/wirble/include mycompiler.c
```

#### Linking Errors

Check library path:

```bash
gcc mycompiler.c -L/path/to/wirble/lib -lwirble-wil -lwirble-common
```

#### Runtime Library Not Found

Set library path:

```bash
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```

## Performance Optimization

### Build Flags

```bash
# CMake
cmake -DCMAKE_BUILD_TYPE=Release -DWIRBLE_ENABLE_LTO=ON ..

# Meson
meson setup build --buildtype=release -Denable_lto=true

# Autotools
./configure CFLAGS="-O3 -flto -march=native"
```

### Profile-Guided Optimization

```bash
# 1. Build with profiling
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-fprofile-generate" ..
cmake --build .

# 2. Run representative workload
./mycompiler test_input.src

# 3. Rebuild with profile data
cmake -DCMAKE_C_FLAGS="-fprofile-use" ..
cmake --build .
```

## Static Analysis

### Clang Static Analyzer

```bash
scan-build cmake ..
scan-build make
```

### Cppcheck

```bash
cppcheck --enable=all src/
```

## Continuous Integration

### GitHub Actions Example

```yaml
name: Build

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      
      - name: Install dependencies
        run: sudo apt-get install -y liblmdb-dev cmake
      
      - name: Configure
        run: cmake -B build -DWIRBLE_BUILD_TESTS=ON
      
      - name: Build
        run: cmake --build build
      
      - name: Test
        run: cd build && ctest --verbose
```

## Best Practices

1. **Use Release Builds for Production**: Debug builds are much slower
2. **Enable LTO**: Link-time optimization improves performance
3. **Link Only What You Need**: Don't link unused components
4. **Use pkg-config**: Simplifies dependency management
5. **Set RPATH**: Avoid LD_LIBRARY_PATH issues
6. **Version Your Dependencies**: Pin WIRBLE version in production
7. **Test on Target Platform**: Cross-compilation can have subtle issues

## Conclusion

WIRBLE's flexible build system and modular design make it easy to integrate into existing projects. Whether using CMake, Meson, or Autotools, WIRBLE can be built and deployed on a wide range of platforms.
