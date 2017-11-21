cmake_minimum_required(VERSION 3.0)
project(bittools)

set(XORFILE_TARGET "xorfile")
set(MIRRORFILE_TARGET "mirrorfile")
set(GETCRCTBL_TARGET "gencrctbl")

set(EXECUTABLE_TARGETS ${XORFILE_TARGET};${MIRRORFILE_TARGET};${GETCRCTBL_TARGET})
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
## environment discover #######################################################
###############################################################################

discover_variable(LINK_TYPE "linkage type")
if (NOT LINK_TYPE)
  set(LINK_TYPE "static")
endif()
discover_variable(BOOST_ROOT "boost")

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
## global definitions #########################################################
###############################################################################

if(WIN32 AND MSVC)
  if (LINK_TYPE STREQUAL "dynamic")
    foreach(flag_var
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
       if(${flag_var} MATCHES "/MT")
          string(REGEX REPLACE "/MT" "/MD" ${flag_var} "${${flag_var}}")
       endif()
    endforeach()
  elseif (LINK_TYPE STREQUAL "static")
    foreach(flag_var
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
       if(${flag_var} MATCHES "/MD")
          string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
       endif()
    endforeach()
  endif()

  # remove optimization parameters from global flags, do control it explicitly per source file or target basis
  remove_global_optimization_flags("*;DEBUG;RELEASE;MINSIZEREL;RELWITHDEBINFO")

  # fix invalid cmake suggestions
  fix_global_flags()
else()
  message(FATAL_ERROR "platform is not implemented")
endif()

print_global_flags()

###############################################################################
## target definitions #########################################################
###############################################################################

# The data is just added to the executable, because in some IDEs (QtCreator) 
# files are invisible when they are not explicitly part of the project.
foreach(target ${EXECUTABLE_TARGETS})
  add_executable(${target} ${sources_common} ${sources_${target}}) #${data_${target}})

  # This allows to include files relative to the root of the src directory with a <> pair
  target_include_directories(${target}
    PUBLIC
      ${CMAKE_CURRENT_LIST_DIR}/includes
      ${CMAKE_CURRENT_LIST_DIR}/src/_common
      ${CMAKE_CURRENT_LIST_DIR}/src
  )

  # local optimization per target basis
  add_target_compile_properties("${target}"
    $<$<CONFIG:RELEASE>:/Ox>  # Full Optimization
    $<$<CONFIG:RELEASE>:/Ob2> # Inline Function Expansion: Any Suitable
    #$<$<CONFIG:RELEASE>:/Oi>  # Enable Intrinsic Functions
    $<$<CONFIG:RELEASE>:/Ot>  # Enable Intrinsic Functions
    $<$<CONFIG:RELEASE>:/GL>  # Whole Program Optimization
    $<$<CONFIG:DEBUG>:/Od>    # Disabled
    /MP
  )
  add_target_link_properties("${EXE_TARGET_NAME}" RELEASE
    "/LTCG" # Use Link Time Code Generation
  )
endforeach()

get_target_property(COMPILE_OPTIONS_${XORFILE_TARGET} ${XORFILE_TARGET} COMPILE_OPTIONS)
get_target_property(LINK_FLAGS_${XORFILE_TARGET}_DEBUG ${XORFILE_TARGET} LINK_FLAGS_DEBUG)
get_target_property(LINK_FLAGS_${XORFILE_TARGET}_RELEASE ${XORFILE_TARGET} LINK_FLAGS_RELEASE)
get_target_property(LINK_FLAGS_${XORFILE_TARGET}_MINSIZEREL ${XORFILE_TARGET} LINK_FLAGS_MINSIZEREL)
get_target_property(LINK_FLAGS_${XORFILE_TARGET}_RELWITHDEBINFO ${XORFILE_TARGET} LINK_FLAGS_RELWITHDEBINFO)

print_flags(
  COMPILE_OPTIONS_${XORFILE_TARGET}
  LINK_FLAGS_${XORFILE_TARGET}_DEBUG
  LINK_FLAGS_${XORFILE_TARGET}_RELEASE
  LINK_FLAGS_${XORFILE_TARGET}_MINSIZEREL
  LINK_FLAGS_${XORFILE_TARGET}_RELWITHDEBINFO
)

## This copies all resource files in the build directory.
## We need this, because we want to work with paths relative to the executable.
#file(COPY ${data} DESTINATION resources)

###############################################################################
## dependencies ###############################################################
###############################################################################

if(LINK_TYPE STREQUAL "dynamic")
  set(Boost_USE_STATIC_LIBS OFF)
  set(Boost_USE_STATIC_RUNTIME OFF)
  set(Boost_USE_MULTITHREADED ON)
elseif (LINK_TYPE STREQUAL "static")
  set(Boost_USE_STATIC_LIBS ON)
  set(Boost_USE_STATIC_RUNTIME ON)
  set(Boost_USE_MULTITHREADED ON)
  #set_target_properties(${EXE_TARGET_NAME} PROPERTIES
  #  COMPILE_OPTIONS -DBOOST_RUNTIME_STATIC_LINK # workaround for unresolved symbols
  #)
endif()

find_package(Boost REQUIRED COMPONENTS filesystem program_options system)
message(STATUS "Boost version: " ${Boost_VERSION} " Location: " ${BOOST_ROOT})

if(Boost_FOUND)
  add_definitions(-DBOOST_ALL_NO_LIB) # Disable auto-linking
  if(LINK_TYPE STREQUAL "dynamic")
    add_definitions(-DBOOST_ALL_DYN_LINK)
  endif()
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

## This is basically a repeat of the file copy instruction that copies the
## resources in the build directory, but here we tell cmake that we want it
## in the package.
install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/res
  DESTINATION $<CONFIGURATION>
  PATTERN "*/~*" EXCLUDE)

# must be after all `install` commands!
foreach(target ${EXECUTABLE_TARGETS})
  install(CODE "
    include(BundleUtilities)
    fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/\${CMAKE_INSTALL_CONFIG_NAME}/${target}.exe\" \"\" \"${Boost_LIBRARY_DIRS}\")
    " COMPONENT ${target})
endforeach()

# Now comes everything we need, to create a package
# there are a lot more variables you can set, and some
# you need to set for some package types, but we want to
# be minimal here.
set(CPACK_PACKAGE_VERSION "1.0.0.0")

#set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${CPACK_PACKAGE_VERSION}-win32-$<CONFIGURATION>")
#set(CPACK_PACKAGE_NAME "${EXE_TARGET_NAME}")

# We don't want to split our program up into several incomplete pieces.
set(CPACK_MONOLITHIC_INSTALL 1)

set(CPACK_OUTPUT_CONFIG_FILE "${CMAKE_BINARY_DIR}/BundleConfig.cmake")

include(CPack)

set(CPACK_BUNDLE_TEMPLATE_CONFIG_FILE "${CMAKE_BINARY_DIR}/CPackConfig.cmake.in")
set(CPACK_BUNDLE_OUTPUT_CONFIG_FILE "${CMAKE_BINARY_DIR}/CPackProperties.cmake")

# make cpack configuration template for later replacements with the expression generator support
file(WRITE "${CPACK_BUNDLE_TEMPLATE_CONFIG_FILE}" "")
file(APPEND "${CPACK_BUNDLE_TEMPLATE_CONFIG_FILE}" "set(CPACK_PACKAGE_FILE_NAME \"\${CPACK_PACKAGE_FILE_NAME}\")\n")

add_custom_target(bundle
  COMMAND ${CMAKE_COMMAND}
    # this one must be written as is, DO NOT put the `$<CONFIGURATION>` inside a variable!
    -D "CPACK_PACKAGE_FILE_NAME=${PROJECT_NAME}-${CPACK_PACKAGE_VERSION}-win32-$<CONFIGURATION>"
    -D "CPACK_BUNDLE_TEMPLATE_CONFIG_FILE=${CPACK_BUNDLE_TEMPLATE_CONFIG_FILE}"
    -D "CPACK_BUNDLE_OUTPUT_CONFIG_FILE=${CPACK_BUNDLE_OUTPUT_CONFIG_FILE}"
    # this one must be after all `-D`s
    -P "${CMAKE_CURRENT_LIST_DIR}/CPackMakeConfig.cmake"
  COMMAND "${CMAKE_CPACK_COMMAND}" 
    -G "NSIS"
    -C "$<CONFIGURATION>"
    --config "${CPACK_OUTPUT_CONFIG_FILE}")
