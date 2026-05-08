# docs/CMakeLists.txt
find_package(Doxygen REQUIRED dot)

if(DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html)
    set(DOXYGEN_INPUT_PATHS
        ${CMAKE_SOURCE_DIR}/include/equinox
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/docs/Manual.i
        ${CMAKE_SOURCE_DIR}/README.md
    )
    set(DOXYGEN_INCLUDE_PATHS
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/third_party
    )
    string(REPLACE ";" " " DOXYGEN_INPUT_PATHS "${DOXYGEN_INPUT_PATHS}")
    string(REPLACE ";" " " DOXYGEN_INCLUDE_PATHS "${DOXYGEN_INCLUDE_PATHS}")

    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

    add_custom_target(docs ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )

    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/
        DESTINATION share/doc/equinox
        OPTIONAL
    )
endif()
