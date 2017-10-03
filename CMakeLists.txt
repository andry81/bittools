cmake_minimum_required(VERSION 3.0)
project(bittools)

set(XORFILE_TARGET "xorfile")
set(MIRRORFILE_TARGET "mirrorfile")

set(EXECUTABLE_TARGETS ${XORFILE_TARGET};${MIRRORFILE_TARGET})
set(ALL_TARGETS ${EXECUTABLE_TARGETS})

if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin)
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/lib)
endif()
if(NOT CPACK_OUTPUT_FILE_PREFIX)
  set(CPACK_OUTPUT_FILE_PREFIX ${CMAKE_CURRENT_LIST_DIR}/pack)
endif()

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(Common)
include(UseEnvironment)

###############################################################################
## file globbing ##############################################################
###############################################################################

# These instructions search the directory tree when cmake is
# invoked and put all files that match the pattern in the variables 
# `sources` and `data`.
file(GLOB_RECURSE sources_common src/*.cpp src/*.h*)

# exclude standalone sources
foreach(target ${ALL_TARGETS})
  exclude_paths_from_path_list(sources_common "${sources_common}" "/src/${target}/" 0)
endforeach()

# exclude tests sources
exclude_paths_from_path_list(sources_common "${sources_common}" "/tests/" 0)

# exclude by pattern
exclude_paths_from_path_list(sources_common "${sources_common}" "/~" 0)

# generate standalone sources groups
foreach(target ${ALL_TARGETS})
  file(GLOB_RECURSE sources_${target} src/${target}/*.cpp src/${target}/*.h*)

  # exclude tests sources
  exclude_paths_from_path_list(sources_${target} "${sources_${target}}" "/tests/" 0)

  # exclude by pattern
  exclude_paths_from_path_list(sources_${target} "${sources_${target}}" "/~" 0)
endforeach()

#file(GLOB_RECURSE headers_test src/tests/*.h*)

# exclude by pattern
exclude_paths_from_path_list(headers_test "${headers_test}" "/~" 0)

#file(GLOB_RECURSE sources_test src/tests/*.cpp ${headers_test})
#
## exclude by pattern
#exclude_paths_from_path_list(sources_test "${sources_test}" "/~" 0)

file(GLOB_RECURSE resources res/*)

# exclude by pattern
exclude_paths_from_path_list(resources "${resources}" "/~" 0)

# You can use set(sources src/main.cpp) etc if you don't want to
# use globbing to find files automatically.

###############################################################################
## target definitions #########################################################
###############################################################################

# The data is just added to the executable, because in some IDEs (QtCreator) 
# files are invisible when they are not explicitly part of the project.
foreach(target ${EXECUTABLE_TARGETS})
  add_executable(${target} ${sources_common} ${sources_${target}}) #${data_${target}})
endforeach()

if(WIN32 AND MSVC)
  # static link: fuk dat shit
  #target_compile_options(${target} PUBLIC /EHsc $<$<CONFIG:RELEASE>:/MT> $<$<CONFIG:DEBUG>:/MTd>)
  foreach(flag_var
          CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
          CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
     if(${flag_var} MATCHES "/MD")
        string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
     endif(${flag_var} MATCHES "/MD")
     message(STATUS "${flag_var}=${${flag_var}}")
  endforeach(flag_var)

  foreach(target ${EXECUTABLE_TARGETS})
    set_target_properties(${target} PROPERTIES
      COMPILE_OPTIONS -DWIN32
      COMPILE_OPTIONS $<$<CONFIG:RELEASE>:-DNDEBUG>
      COMPILE_OPTIONS $<$<CONFIG:DEBUG>:-DDEBUG>
      COMPILE_OPTIONS -D_CONSOLE
      COMPILE_OPTIONS /EHsc
      COMPILE_OPTIONS $<$<CONFIG:RELEASE>:/MT> # static runtime
      COMPILE_OPTIONS $<$<CONFIG:DEBUG>:/MTd>  # static runtime
      COMPILE_OPTIONS /MP
      COMPILE_OPTIONS /W3
      LINK_FLAGS /INCREMENTAL:NO
      LINK_FLAGS /OPT:REF
      LINK_FLAGS $<$<CONFIG:DEBUG>:/DEBUG>
      LINK_FLAGS /SUBSYSTEM:CONSOLE
    )
  endforeach()
else()
  message(FATAL_ERROR "platform is not implemented")
endif()

# This allows to include files relative to the root of the src directory with a <> pair
foreach(target ${ALL_TARGETS})
  target_include_directories(${target}
    PUBLIC
      ${CMAKE_CURRENT_LIST_DIR}/includes
      ${CMAKE_CURRENT_LIST_DIR}/src/_common
  )
endforeach()

## This copies all resource files in the build directory.
## We need this, because we want to work with paths relative to the executable.
#file(COPY ${data} DESTINATION resources)

###############################################################################
## dependencies ###############################################################
###############################################################################

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost REQUIRED COMPONENTS filesystem program_options system)
message(STATUS "Boost version: " ${Boost_VERSION} " Location: " ${BOOST_ROOT})

if(Boost_FOUND)
  add_definitions(-DBOOST_ALL_NO_LIB) # Disable auto-linking
  add_definitions(-DBOOST_SCOPE_EXIT_CONFIG_USE_LAMBDAS) # Force to use C++11 lambda functions to implement scope exits.

  foreach(target ${ALL_TARGETS})
    target_include_directories(${target}
      PUBLIC
        ${Boost_INCLUDE_DIRS}
    )
  endforeach()

  link_directories(${Boost_LIBRARY_DIRS})
  foreach(target ${ALL_TARGETS})
    target_link_libraries(${target}
      PUBLIC
        ${Boost_LIBRARIES}
    )
  endforeach()
endif()

###############################################################################
## packaging ##################################################################
###############################################################################

# All install commands get the same destination. this allows us to use paths
# relative to the executable.
install(TARGETS ${ALL_TARGETS} DESTINATION $<CONFIGURATION>)

### This is basically a repeat of the file copy instruction that copies the
### resources in the build directory, but here we tell cmake that we want it
### in the package.
#install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/res DESTINATION $<CONFIGURATION>)

# must be after all `install` commands!
foreach(target ${EXECUTABLE_TARGETS})
  install(CODE "
    include(BundleUtilities)
    fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/\${CMAKE_INSTALL_CONFIG_NAME}/${target}.exe\" \"\" \"${Boost_LIBRARY_DIRS}\")
    " COMPONENT ${target})
endforeach()
