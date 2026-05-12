 # HandleExamples.cmake
 # Manages example files for WIRBLE substrates
 
 # Function to install examples for a substrate
 function(wirble_install_examples substrate_name)
   set(example_dir ${CMAKE_CURRENT_SOURCE_DIR}/examples/${substrate_name})
   
   if(EXISTS ${example_dir})
     file(GLOB example_files "${example_dir}/*.${substrate_name}")
     
     if(example_files)
       install(
         FILES ${example_files}
         DESTINATION ${CMAKE_INSTALL_DATADIR}/wirble/examples/${substrate_name}
       )
       message(STATUS "Found ${substrate_name} examples: ${example_files}")
     endif()
   endif()
 endfunction()
 
 # Install examples for all substrates
 if(INSTALL_EXAMPLES)
   wirble_install_examples(mal)
   wirble_install_examples(mds)
   wirble_install_examples(tos)
   wirble_install_examples(vxt)
   wirble_install_examples(wil)
   wirble_install_examples(wrs)
   wirble_install_examples(wvm)
   
   # Also install common examples
   install(
     DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/examples/common
     DESTINATION ${CMAKE_INSTALL_DATADIR}/wirble/examples
     FILES_MATCHING PATTERN "*"
   )
   
   # Install stage11.expr
   if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/examples/stage11.expr)
     install(
       FILES ${CMAKE_CURRENT_SOURCE_DIR}/examples/stage11.expr
       DESTINATION ${CMAKE_INSTALL_DATADIR}/wirble/examples
     )
   endif()
 endif()
 
 # Add option to enable example installation
 option(INSTALL_EXAMPLES "Install WIRBLE example files" OFF)
 
 # Function to add example tests
 function(wirble_add_example_tests)
   if(BUILD_TESTING)
     # Test that examples can be parsed/validated
     set(substrates mal mds tos vxt wil wrs wvm)
     
     foreach(substrate ${substrates})
       set(example_dir ${CMAKE_CURRENT_SOURCE_DIR}/examples/${substrate})
       
       if(EXISTS ${example_dir})
         file(GLOB substrate_examples "${example_dir}/*.${substrate}")
         
         foreach(example_file ${substrate_examples})
           get_filename_component(example_name ${example_file} NAME_WE)
           
           # Add test to validate example syntax (if validator exists)
           if(TARGET ${substrate}-validate)
             add_test(
               NAME validate_${substrate}_${example_name}
               COMMAND ${substrate}-validate ${example_file}
             )
           endif()
         endforeach()
       endif()
     endforeach()
   endif()
 endfunction()
 
 # Generate example documentation
 function(wirble_generate_example_docs)
   set(doc_output ${CMAKE_CURRENT_BINARY_DIR}/examples_index.md)
   
   file(WRITE ${doc_output} "# WIRBLE Examples\n\n")
   file(APPEND ${doc_output} "This directory contains examples for each WIRBLE substrate.\n\n")
   
   set(substrates mal mds tos vxt wil wrs wvm)
   
   foreach(substrate ${substrates})
     set(example_dir ${CMAKE_CURRENT_SOURCE_DIR}/examples/${substrate})
     
     if(EXISTS ${example_dir})
       file(GLOB substrate_examples "${example_dir}/*.${substrate}")
       list(LENGTH substrate_examples example_count)
       
       if(example_count GREATER 0)
         file(APPEND ${doc_output} "## ${substrate} (${example_count} examples)\n\n")
         
         foreach(example_file ${substrate_examples})
           get_filename_component(example_name ${example_file} NAME)
           file(APPEND ${doc_output} "- `${example_name}`\n")
         endforeach()
         
         file(APPEND ${doc_output} "\n")
       endif()
     endif()
   endforeach()
   
   message(STATUS "Generated example documentation: ${doc_output}")
 endfunction()
 
 # Call documentation generator
 wirble_generate_example_docs()
