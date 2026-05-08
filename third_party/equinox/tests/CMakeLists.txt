# Find or fetch Catch2
find_package(Catch2 3 QUIET)

if(NOT Catch2_FOUND)
    message(STATUS "Catch2 not found, fetching from GitHub...")
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.2
    )
    FetchContent_MakeAvailable(Catch2)
endif()

# Collect all test source files that actually exist
file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test_*.c")

if(NOT TEST_SOURCES)
    message(WARNING "No test files found in ${CMAKE_CURRENT_SOURCE_DIR}")
    return()
endif()

# Create test executable for each test file
foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    set_source_files_properties(${test_src} PROPERTIES LANGUAGE CXX)
    add_executable(${test_name} ${test_src})
    
    # Link against equinox library
    if(TARGET equinox_shared)
        target_link_libraries(${test_name} PRIVATE equinox_shared Catch2::Catch2WithMain)
    elseif(TARGET equinox_static)
        target_link_libraries(${test_name} PRIVATE equinox_static Catch2::Catch2WithMain)
    else()
        message(FATAL_ERROR "No equinox library target found")
    endif()
    
    # Add to CTest
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

# Optional: Create single test runner only if we have test sources
if(TEST_SOURCES)
    add_executable(run_all_tests ${TEST_SOURCES})
    
    if(TARGET equinox_shared)
        target_link_libraries(run_all_tests PRIVATE equinox_shared Catch2::Catch2WithMain)
    elseif(TARGET equinox_static)
        target_link_libraries(run_all_tests PRIVATE equinox_static Catch2::Catch2WithMain)
    endif()
    
    add_test(NAME all_tests COMMAND run_all_tests)
endif()
