 # HandleIncludes.cmake
 # Manages include directories for WIRBLE
 
 # Public include directories (installed headers)
 set(WIRBLE_PUBLIC_INCLUDES
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/dependencies
 )
 
 # Private include directories (internal headers)
 set(WIRBLE_PRIVATE_INCLUDES
   ${CMAKE_CURRENT_SOURCE_DIR}/src/wirble
   ${CMAKE_CURRENT_SOURCE_DIR}/src/wirble/common
   ${CMAKE_CURRENT_SOURCE_DIR}/src/wirble/support
   ${CMAKE_CURRENT_SOURCE_DIR}/src/wirble/wil
   ${CMAKE_CURRENT_SOURCE_DIR}/src/wirble/wrs
   ${CMAKE_CURRENT_SOURCE_DIR}/src/wirble/mal
   ${CMAKE_CURRENT_SOURCE_DIR}/src/wirble/mds
   ${CMAKE_CURRENT_SOURCE_DIR}/src/wirble/tos
   ${CMAKE_CURRENT_SOURCE_DIR}/src/wirble/vxt
   ${CMAKE_CURRENT_SOURCE_DIR}/src/wirble/wvm
   ${CMAKE_CURRENT_BINARY_DIR}
 )
 
 # Third-party include directories
 set(WIRBLE_THIRD_PARTY_INCLUDES
   ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dparser
   ${CMAKE_CURRENT_SOURCE_DIR}/third_party/microrl/src
   ${CMAKE_CURRENT_SOURCE_DIR}/third_party/equinox/include
   ${CMAKE_CURRENT_SOURCE_DIR}/third_party/equinox/third_party/uthash
   ${CMAKE_CURRENT_SOURCE_DIR}/third_party/equinox/third_party/tlsf
   ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sfsexp
 )
 
 # Function to add WIRBLE includes to a target
 function(wirble_target_include_directories target_name)
   target_include_directories(
     ${target_name}
     PUBLIC
       ${WIRBLE_PUBLIC_INCLUDES}
     PRIVATE
       ${WIRBLE_PRIVATE_INCLUDES}
       ${WIRBLE_THIRD_PARTY_INCLUDES}
   )
 endfunction()
 
 # Install public headers
 install(
   DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/wirble
   DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
   FILES_MATCHING PATTERN "*.h"
 )
 
 install(
   DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/
   DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
   FILES_MATCHING PATTERN "*.h"
 )
