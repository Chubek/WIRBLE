 # HandleThirdParty.cmake
 # Manages third-party dependencies for WIRBLE
 
 # DParser library
 add_library(
   wirble_third_party_dparse
   STATIC
     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dparser/arg.c
     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dparser/parse.c
     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dparser/scan.c
     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dparser/dsymtab.c
     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dparser/util.c
     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dparser/read_binary.c
     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dparser/dparse_tree.c
     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dparser/version.c
 )
 target_compile_definitions(
   wirble_third_party_dparse
   PRIVATE
     D_MAJOR_VERSION=1
     D_MINOR_VERSION=30
 )
 target_include_directories(
   wirble_third_party_dparse
   PUBLIC
     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dparser
 )
 
 # MkDParse library
 add_library(
   wirble_third_party_mkdparse
   STATIC
     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dparser/mkdparse.c
     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dparser/write_tables.c
     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dparser/grammar.g.c
     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dparser/gram.c
     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dparser/lex.c
     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dparser/lr.c
     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dparser/version.c
 )
 target_compile_definitions(
   wirble_third_party_mkdparse
   PRIVATE
     D_MAJOR_VERSION=1
     D_MINOR_VERSION=30
 )
 target_include_directories(
   wirble_third_party_mkdparse
   PUBLIC
     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dparser
 )
 target_link_libraries(wirble_third_party_mkdparse PUBLIC wirble_third_party_dparse)
 
 # Make DParser executable
 add_executable(
   wirble_make_dparser
   ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dparser/make_dparser.c
   ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dparser/version.c
 )
 target_compile_definitions(
   wirble_make_dparser
   PRIVATE
     D_MAJOR_VERSION=1
     D_MINOR_VERSION=30
 )
 target_include_directories(
   wirble_make_dparser
   PRIVATE
     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dparser
 )
 target_link_libraries(
   wirble_make_dparser
   PRIVATE
     wirble_third_party_mkdparse
     wirble_third_party_dparse
 )
 
 # MicroRL library
 add_library(
   wirble_third_party_microrl
   STATIC
     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/microrl/src/microrl.c
 )
 target_include_directories(
   wirble_third_party_microrl
   PUBLIC
     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/microrl/src
 )
 
 # Equinox library
 add_library(
   wirble_third_party_equinox
   STATIC
     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/equinox/src/equinox.c
     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/equinox/src/equinox_arena.c
     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/equinox/src/equinox_vm.c
 )
 target_include_directories(
   wirble_third_party_equinox
   PUBLIC
     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/equinox/include
     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/equinox/third_party/uthash
     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/equinox/third_party/tlsf
 )
 
 # SFSExp library
 add_library(
   wirble_third_party_sfsexp
   STATIC
     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sfsexp/sfsexp.c
 )
 target_include_directories(
   wirble_third_party_sfsexp
   PUBLIC
     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sfsexp
 )
