file(GLOB FUZZ_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/fuzz_*.c")

if(NOT FUZZ_SOURCES)
    message(WARNING "No fuzz harnesses found in ${CMAKE_CURRENT_SOURCE_DIR}")
    return()
endif()

add_custom_target(fuzz_targets ALL DEPENDS ${FUZZ_SOURCES})

foreach(fuzz_src ${FUZZ_SOURCES})
    get_filename_component(fuzz_base ${fuzz_src} NAME)

    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${fuzz_base}.o"
        COMMAND ${CMAKE_C_COMPILER}
                -std=c${CMAKE_C_STANDARD}
                -I${CMAKE_CURRENT_SOURCE_DIR}/../include
                -I${CMAKE_CURRENT_SOURCE_DIR}/../third_party/uthash/src
                -I${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tlsf
                -I${CMAKE_CURRENT_SOURCE_DIR}/../third_party/libdag-ethash
                -c ${fuzz_src}
                -o "${CMAKE_CURRENT_BINARY_DIR}/${fuzz_base}.o"
        DEPENDS ${fuzz_src}
        COMMENT "Compiling fuzz object ${fuzz_base}"
    )

    list(APPEND FUZZ_OBJECTS "${CMAKE_CURRENT_BINARY_DIR}/${fuzz_base}.o")
endforeach()

add_custom_target(fuzz_objects DEPENDS ${FUZZ_OBJECTS})
add_dependencies(fuzz_targets fuzz_objects)
