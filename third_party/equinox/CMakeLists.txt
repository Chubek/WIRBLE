cmake_minimum_required(VERSION 3.10)
project(equinox VERSION 1.0.0 LANGUAGES C CXX)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build options
option(EQX_BUILD_TESTS "Build tests (legacy alias for EQX_BUILD_UNITTEST)" OFF)
option(EQX_BUILD_UNITTEST "Build unit tests" OFF)
option(EQX_BUILD_FUZZ "Build fuzz harnesses" OFF)
option(EQX_BUILD_DOCS "Build documentation" ON)
option(EQX_BUILD_SHARED "Build shared library" ON)
option(EQX_BUILD_STATIC "Build static library" ON)

# Compiler flags
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/uthash/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tlsf
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libdag-ethash/
)

# Third-party libraries
# uthash is header-only, no library to link.
add_library(tlsf STATIC third_party/tlsf/tlsf.c)
set_target_properties(tlsf PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/third_party/tlsf
)
target_include_directories(tlsf PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tlsf
)

set(TLSF_LIBRARY $<TARGET_FILE:tlsf>)
set(DAG_ETHASH_LIBRARY "")

# Source files
set(EQUINOX_SOURCES
    src/enode.c
    src/eclass.c
    src/egraph.c
    src/unionfind.c
    src/hashcons.c
    src/rewrite.c
)

# Header files
set(EQUINOX_HEADERS
    include/equinox/equinox.h
    include/equinox/enode.h
    include/equinox/eclass.h
    include/equinox/egraph.h
    include/equinox/unionfind.h
    include/equinox/hashcons.h
    include/equinox/rewrite.h
    include/equinox/libutils.h
)

# Build shared library
if(EQX_BUILD_SHARED)
    add_library(equinox_shared SHARED ${EQUINOX_SOURCES})
    set_target_properties(equinox_shared PROPERTIES
        OUTPUT_NAME equinox
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
    )
    target_link_libraries(equinox_shared
        PRIVATE ${TLSF_LIBRARY} ${DAG_ETHASH_LIBRARY}
    )
    target_include_directories(equinox_shared PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    install(TARGETS equinox_shared
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
    )
endif()

# Build static library
if(EQX_BUILD_STATIC)
    add_library(equinox_static STATIC ${EQUINOX_SOURCES})
    set_target_properties(equinox_static PROPERTIES
        OUTPUT_NAME equinox
        VERSION ${PROJECT_VERSION}
    )
    target_link_libraries(equinox_static
        PRIVATE ${TLSF_LIBRARY} ${DAG_ETHASH_LIBRARY}
    )
    target_include_directories(equinox_static PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    install(TARGETS equinox_static
        ARCHIVE DESTINATION lib
    )
endif()

# Install headers
install(FILES ${EQUINOX_HEADERS}
    DESTINATION include/equinox
)

# pkg-config file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/equinox.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/equinox.pc
    @ONLY
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/equinox.pc
    DESTINATION lib/pkgconfig
)

if(EQX_BUILD_TESTS)
    set(EQX_BUILD_UNITTEST ON)
endif()

 
if(EQX_BUILD_UNITTEST)
    enable_testing()
    add_subdirectory(tests)
endif()

if(EQX_BUILD_FUZZ)
    add_subdirectory(fuzz)
endif()

# Documentation
if(EQX_BUILD_DOCS)
    add_subdirectory(docs)
endif()

# Summary
message(STATUS "")
message(STATUS "Equinox Configuration Summary:")
message(STATUS "  Version:        ${PROJECT_VERSION}")
message(STATUS "  C Standard:     C${CMAKE_C_STANDARD}")
message(STATUS "  Build shared:   ${EQX_BUILD_SHARED}")
message(STATUS "  Build static:   ${EQX_BUILD_STATIC}")
message(STATUS "  Build tests:    ${EQX_BUILD_UNITTEST}")
message(STATUS "  Build fuzz:     ${EQX_BUILD_FUZZ}")
message(STATUS "  Build docs:     ${EQX_BUILD_DOCS}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
