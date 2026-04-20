if(INSTALL_MANIFEST)
  install(
    FILES
      ${CMAKE_CURRENT_SOURCE_DIR}/SourceManifest.yaml
      ${CMAKE_CURRENT_SOURCE_DIR}/ToolchainManifest.yaml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/wirble/manifests
  )

  install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/api/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/wirble/manifests/api
    FILES_MATCHING
      PATTERN "*.zspec"
  )

  install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/grammars/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/wirble/manifests/grammars
    FILES_MATCHING
      PATTERN "*.ebnf"
  )

  install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/schema/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/wirble/manifests/schema
    FILES_MATCHING
      PATTERN "*.json"
      PATTERN "*.xsd"
      PATTERN "*.yaml"
  )
endif()
