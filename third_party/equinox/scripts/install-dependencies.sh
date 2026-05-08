#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
THIRD_PARTY_DIR="$PROJECT_ROOT/third_party"

echo "Installing third-party dependencies for Equinox..."

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# uthash is header-only, already vendored
echo -e "${GREEN}✓${NC} uthash (header-only, already present)"

# tlsf is already vendored
echo -e "${GREEN}✓${NC} tlsf (already present)"

# libdag-ethash is already vendored
echo -e "${GREEN}✓${NC} libdag-ethash (already present)"

# Create CMakeLists.txt for uthash (header-only)
cat > "$THIRD_PARTY_DIR/uthash/CMakeLists.txt" << 'EOF'
cmake_minimum_required(VERSION 3.10)
project(uthash LANGUAGES C)

# Header-only library
add_library(uthash INTERFACE)
target_include_directories(uthash INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

# Install headers
install(DIRECTORY src/
    DESTINATION include/uthash
    FILES_MATCHING PATTERN "*.h"
)
EOF

echo -e "${GREEN}✓${NC} Created CMakeLists.txt for uthash"

# Create CMakeLists.txt for tlsf
cat > "$THIRD_PARTY_DIR/tlsf/CMakeLists.txt" << 'EOF'
cmake_minimum_required(VERSION 3.10)
project(tlsf LANGUAGES C)

add_library(tlsf STATIC tlsf.c)
target_include_directories(tlsf PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Install
install(TARGETS tlsf
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
install(FILES tlsf.h DESTINATION include)
EOF

echo -e "${GREEN}✓${NC} Created CMakeLists.txt for tlsf"

# Create CMakeLists.txt for libdag-ethash
cat > "$THIRD_PARTY_DIR/libdag-ethash/CMakeLists.txt" << 'EOF'
cmake_minimum_required(VERSION 3.10)
project(libdag-ethash LANGUAGES C)

set(LIBDAG_SOURCES
    blake2b-ref.c
    dag.c
    dagalgo.c
    dagio.c
    keccak.c
    mdag.c
    mine.c
    mixone.c
    target.c
    util.c
)

add_library(dag-ethash STATIC ${LIBDAG_SOURCES})
target_include_directories(dag-ethash PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Install
install(TARGETS dag-ethash
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
install(FILES
    blake2.h
    blake2-impl.h
    dag.h
    dagalgo.h
    dagio.h
    keccak.h
    mdag.h
    mine.h
    target.h
    util.h
    DESTINATION include/libdag-ethash
)
EOF

echo -e "${GREEN}✓${NC} Created CMakeLists.txt for libdag-ethash"

echo ""
echo -e "${GREEN}All dependencies configured successfully!${NC}"
echo ""
echo "You can now build the project:"
echo "  mkdir -p build && cd build"
echo "  cmake .."
echo "  make"
