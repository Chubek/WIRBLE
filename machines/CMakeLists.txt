if(NOT NO_INSTALL_MACHINES)
  install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/wirble/manifests/machines
    FILES_MATCHING
      PATTERN "*.json"
      PATTERN "*.yaml"
      PATTERN "*.xml"
  )
endif()
