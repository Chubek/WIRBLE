include(GNUInstallDirs)

find_package(Doxygen QUIET)

set(WIRBLE_DOXYGEN_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/doxygen")
set(WIRBLE_DOXYGEN_CONFIG "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile")
set(WIRBLE_DOXYGEN_HTML_DIR "${WIRBLE_DOXYGEN_OUTPUT_DIR}/html")
set(WIRBLE_DOXYGEN_LATEX_DIR "${WIRBLE_DOXYGEN_OUTPUT_DIR}/latex")
set(WIRBLE_DOXYGEN_MAN_DIR "${WIRBLE_DOXYGEN_OUTPUT_DIR}/man")
set(WIRBLE_DOXYGEN_WARN_LOG "${CMAKE_CURRENT_BINARY_DIR}/doxygen-warnings.log")
set(WIRBLE_DOC_STAMP "${CMAKE_CURRENT_BINARY_DIR}/doxygen.stamp")
set(WIRBLE_PROJECT_VERSION "${PROJECT_VERSION}")
set(WIRBLE_TOP_SRCDIR "${CMAKE_SOURCE_DIR}")
set(WIRBLE_OUTPUT_DIRECTORY "${WIRBLE_DOXYGEN_OUTPUT_DIR}")
set(WIRBLE_LAYOUT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/layout.xml")
set(WIRBLE_STRIP_FROM_INC_PATH "${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src")
set(WIRBLE_WARN_LOGFILE "${WIRBLE_DOXYGEN_WARN_LOG}")

if(NOT DEFINED INSTALL_DOCS)
  set(INSTALL_DOCS OFF)
endif()

if(NOT DEFINED INSTALL_MANFILES)
  set(INSTALL_MANFILES OFF)
endif()

set(WIRBLE_DOC_TARGETS)

if(DOXYGEN_FOUND)
  set(WIRBLE_HAVE_DOT NO)
  if(DOXYGEN_DOT_FOUND)
    set(WIRBLE_HAVE_DOT YES)
  endif()

  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
    ${WIRBLE_DOXYGEN_CONFIG}
    @ONLY
  )

  add_custom_command(
    OUTPUT ${WIRBLE_DOC_STAMP}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${WIRBLE_DOXYGEN_OUTPUT_DIR}
    COMMAND ${DOXYGEN_EXECUTABLE} ${WIRBLE_DOXYGEN_CONFIG}
    COMMAND ${CMAKE_COMMAND} -E touch ${WIRBLE_DOC_STAMP}
    DEPENDS
      ${WIRBLE_DOXYGEN_CONFIG}
      ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
      ${CMAKE_CURRENT_SOURCE_DIR}/layout.xml
      ${CMAKE_SOURCE_DIR}/README.md
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    VERBATIM
  )

  add_custom_target(wirble-docs
    DEPENDS ${WIRBLE_DOC_STAMP}
  )
  list(APPEND WIRBLE_DOC_TARGETS wirble-docs)

  find_program(WIRBLE_MAKE_PROGRAM NAMES ${CMAKE_MAKE_PROGRAM} gmake make)
  if(WIRBLE_MAKE_PROGRAM)
    add_custom_command(
      OUTPUT ${WIRBLE_DOXYGEN_LATEX_DIR}/refman.pdf
      COMMAND ${WIRBLE_MAKE_PROGRAM} -C ${WIRBLE_DOXYGEN_LATEX_DIR} pdf
      DEPENDS ${WIRBLE_DOC_STAMP}
      WORKING_DIRECTORY ${WIRBLE_DOXYGEN_LATEX_DIR}
      VERBATIM
    )
    add_custom_target(wirble-docs-pdf DEPENDS ${WIRBLE_DOXYGEN_LATEX_DIR}/refman.pdf)
    add_dependencies(wirble-docs-pdf wirble-docs)
    list(APPEND WIRBLE_DOC_TARGETS wirble-docs-pdf)
  endif()

  if(INSTALL_DOCS)
    install(CODE
      "execute_process(COMMAND \"${CMAKE_COMMAND}\" --build \"${CMAKE_BINARY_DIR}\" --target wirble-docs RESULT_VARIABLE wirble_docs_result)\nif(NOT wirble_docs_result EQUAL 0)\n  message(FATAL_ERROR \"Failed to build documentation target wirble-docs\")\nendif()"
    )
    install(DIRECTORY ${WIRBLE_DOXYGEN_HTML_DIR}/ DESTINATION ${CMAKE_INSTALL_DOCDIR}/html OPTIONAL)
    install(DIRECTORY ${WIRBLE_DOXYGEN_LATEX_DIR}/ DESTINATION ${CMAKE_INSTALL_DOCDIR}/latex OPTIONAL)

    if(TARGET wirble-docs-pdf)
      install(CODE
        "execute_process(COMMAND \"${CMAKE_COMMAND}\" --build \"${CMAKE_BINARY_DIR}\" --target wirble-docs-pdf RESULT_VARIABLE wirble_docs_pdf_result)\nif(NOT wirble_docs_pdf_result EQUAL 0)\n  message(FATAL_ERROR \"Failed to build documentation target wirble-docs-pdf\")\nendif()"
      )
      install(FILES ${WIRBLE_DOXYGEN_LATEX_DIR}/refman.pdf DESTINATION ${CMAKE_INSTALL_DOCDIR}/pdf OPTIONAL)
    endif()
  endif()

  if(INSTALL_MANFILES)
    install(CODE
      "execute_process(COMMAND \"${CMAKE_COMMAND}\" --build \"${CMAKE_BINARY_DIR}\" --target wirble-docs RESULT_VARIABLE wirble_man_result)\nif(NOT wirble_man_result EQUAL 0)\n  message(FATAL_ERROR \"Failed to build documentation target wirble-docs for manpages\")\nendif()"
    )
    install(DIRECTORY ${WIRBLE_DOXYGEN_MAN_DIR}/man3wil/ DESTINATION ${CMAKE_INSTALL_MANDIR}/man3wil OPTIONAL)
  endif()
else()
  message(STATUS "Doxygen not found: documentation targets are disabled")
endif()
