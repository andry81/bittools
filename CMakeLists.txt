cmake_minimum_required(VERSION 3.0)

###############################################################################
## cmake policy change ########################################################
###############################################################################

#  Policy CMP0057 is not set: Support new IN_LIST if() operator.  Run "cmake
#  --help-policy CMP0057" for policy details.  Use the cmake_policy command to
#  set the policy and suppress this warning.
#
#  IN_LIST will be interpreted as an operator when the policy is set to NEW.
#  Since the policy is not set the OLD behavior will be used.
cmake_policy(SET CMP0057 NEW) # cmake >= 3.3

###############################################################################
## cmake builtin search paths and includes ####################################
###############################################################################

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

include(Common)
include(Qt)

###############################################################################
## discover environment variables #############################################
###############################################################################

configure_environment()

discover_variable(LINK_TYPE STRING "linkage type")
if (NOT LINK_TYPE)
  set(LINK_TYPE "static") # linkage by default
endif()

discover_variable(_3DPARTY_LOCAL_ROOT PATH "local 3dparty root")
discover_variable(NSIS_INSTALL_ROOT PATH "cpack nsis intall root")
discover_variable(BOOST_ROOT PATH "boost")

###############################################################################
## projects description #######################################################
###############################################################################

project(bittools)

set(XORFILE_TARGET "xorfile")
set(MIRRORFILE_TARGET "mirrorfile")
set(GENCRCTBL_TARGET "gencrctbl")

set(ALL_TARGETS ${XORFILE_TARGET};${MIRRORFILE_TARGET};${GENCRCTBL_TARGET})

if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin)
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/lib)
endif()
if(NOT CPACK_OUTPUT_FILE_PREFIX)
  set(CPACK_OUTPUT_FILE_PREFIX ${CMAKE_CURRENT_LIST_DIR}/pack)
endif()

###############################################################################
## file globbing ##############################################################
###############################################################################

# These instructions search the directory tree when cmake is
# invoked and put all files that match the pattern in respective variables.
file(GLOB_RECURSE all_sources
  ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
  ${CMAKE_CURRENT_LIST_DIR}/includes/*.h*
  ${CMAKE_CURRENT_LIST_DIR}/src/*.h*)

set(common_sources ${all_sources})

# exclude standalone sources
foreach(target ${ALL_TARGETS})
  exclude_paths_from_path_list(common_sources "${common_sources}" "/src/${target}/" 0)
endforeach()

# exclude tests sources
exclude_paths_from_path_list(common_sources "${common_sources}" "/tests/" 0)

# exclude by pattern
exclude_paths_from_path_list(common_sources "${common_sources}" "/~" 0)

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

# You can use set(sources src/main.cpp) etc if you don't want to
# use globbing to find files automatically.

###############################################################################
## global definitions #########################################################
###############################################################################

if(WIN32 AND MSVC)
  # set global linkage type (dynamic/static)
  set_global_link_type(${LINK_TYPE})

  # remove optimization parameters from global flags, do control it explicitly per source file or target basis
  remove_global_optimization_flags("*;DEBUG;RELEASE;MINSIZEREL;RELWITHDEBINFO")

  # fix invalid cmake suggestions
  fix_global_flags()
else()
  message(FATAL_ERROR "platform is not implemented")
endif()

print_global_flags()

###############################################################################
## dynamic dependencies #######################################################
###############################################################################

# boost

if(LINK_TYPE STREQUAL "dynamic")
  set(Boost_USE_STATIC_LIBS OFF)
  set(Boost_USE_STATIC_RUNTIME OFF)
  set(Boost_USE_MULTITHREADED ON)
elseif (LINK_TYPE STREQUAL "static")
  set(Boost_USE_STATIC_LIBS ON)
  set(Boost_USE_STATIC_RUNTIME ON)
  set(Boost_USE_MULTITHREADED ON)
endif()

find_package(Boost REQUIRED COMPONENTS filesystem program_options system)
message(STATUS "(*) Found `Boost`: ${Boost_VERSION} Location: \"${BOOST_ROOT}\" Libs: [${Boost_LIBRARIES}]")

if(Boost_FOUND)
  add_definitions(-DBOOST_ALL_NO_LIB) # Disable auto-linking
  if(LINK_TYPE STREQUAL "dynamic")
    add_definitions(-DBOOST_ALL_DYN_LINK)
  endif()
  add_definitions(-DBOOST_SCOPE_EXIT_CONFIG_USE_LAMBDAS) # Force to use C++11 lambda functions to implement scope exits.
endif()

###############################################################################
## target definitions #########################################################
###############################################################################

# The data is just added to the executable, because in some IDEs (QtCreator) 
# files are invisible when they are not explicitly part of the project.
foreach(target ${ALL_TARGETS})
  add_executable(${target} ${common_sources} ${sources_${target}}) #${data_${target}})

  # This allows to include files relative to the root of the src directory with a <> pair
  target_include_directories(${target}
    PUBLIC
      ${CMAKE_CURRENT_LIST_DIR}/src
      ${CMAKE_CURRENT_LIST_DIR}/src/_common
      ${CMAKE_CURRENT_LIST_DIR}/includes
      ${Boost_INCLUDE_DIRS}
  )

  link_directories(${Boost_LIBRARY_DIRS})

  target_link_libraries(${target}
    PUBLIC
      ${Boost_LIBRARIES}
  )
endforeach()

###############################################################################
## target optimization ########################################################
###############################################################################

# local optimization per target basis
add_target_compile_properties("${ALL_TARGETS}"
  $<$<CONFIG:RELEASE>:/Ox>  # Full Optimization
  $<$<CONFIG:RELEASE>:/Ob2> # Inline Function Expansion: Any Suitable
  #$<$<CONFIG:RELEASE>:/Oi>  # Enable Intrinsic Functions
  $<$<CONFIG:RELEASE>:/Ot>  # Enable Intrinsic Functions
  $<$<CONFIG:RELEASE>:/GL>  # Whole Program Optimization
  $<$<CONFIG:DEBUG>:/Od>    # Disabled
  /MP
)
add_target_link_properties("${ALL_TARGETS}" RELEASE
  "/LTCG" # Use Link Time Code Generation
)

get_target_property(COMPILE_OPTIONS_${XORFILE_TARGET} ${XORFILE_TARGET} COMPILE_OPTIONS)
get_target_property(LINK_FLAGS_${XORFILE_TARGET}_DEBUG ${XORFILE_TARGET} LINK_FLAGS_DEBUG)
get_target_property(LINK_FLAGS_${XORFILE_TARGET}_RELEASE ${XORFILE_TARGET} LINK_FLAGS_RELEASE)
get_target_property(LINK_FLAGS_${XORFILE_TARGET}_MINSIZEREL ${XORFILE_TARGET} LINK_FLAGS_MINSIZEREL)
get_target_property(LINK_FLAGS_${XORFILE_TARGET}_RELWITHDEBINFO ${XORFILE_TARGET} LINK_FLAGS_RELWITHDEBINFO)

###############################################################################
## target flags ###############################################################
###############################################################################

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
## packaging ##################################################################
###############################################################################

# All install commands get the same destination. this allows us to use paths
# relative to the executable.
install(TARGETS ${ALL_TARGETS} DESTINATION $<CONFIGURATION>)

### This is basically a repeat of the file copy instruction that copies the
### resources in the build directory, but here we tell cmake that we want it
### in the package.
#install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/res
#  DESTINATION $<CONFIGURATION>
#  PATTERN "*/~*" EXCLUDE)

# must be after all `install` commands!
foreach(target ${ALL_TARGETS})
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
    -P "${CMAKE_CURRENT_LIST_DIR}/cmake/CPackMakeConfig.cmake"
  COMMAND "${CMAKE_CPACK_COMMAND}" 
    -G "NSIS"
    -C "$<CONFIGURATION>"
    --config "${CPACK_OUTPUT_CONFIG_FILE}")
