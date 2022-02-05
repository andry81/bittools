cmake_minimum_required(VERSION 3.7)

# at least cmake 3.7 is required for:
# * to request source directory path from target:
#   `get_target_property(FOO_SOURCE_DIR Foo SOURCE_DIR)`
# * to request BUILDSYSTEM_TARGETS property from a directory.
#

# at least cmake 3.3 is required for:
# * to use IN_LIST in if command:
#   (https://cmake.org/cmake/help/v3.3/command/if.html )
#   `if(<variable|string> IN_LIST <variable>)`
#

###############################################################################
## cmake policy change ########################################################
###############################################################################

#if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "X.Y.Z")
#  #  Policy CMP00NN is not set: ...
#  #
#  #  ...
#  cmake_policy(SET CMP00NN NEW) # cmake >= X.Y
#endif()

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.1.0")
  #  Policy CMP0054 is not set: Only interpret if () arguments as variables or
  #  keywords when unquoted.  Run "cmake --help-policy CMP0054" for policy
  #  details.  Use the cmake_policy command to set the policy and suppress this
  #  warning.
  #
  #  Quoted variables like "MSVC" will no longer be dereferenced when the
  #  policy is set to NEW.  Since the policy is not set the OLD behavior will
  #  be used.
  cmake_policy(SET CMP0054 NEW) # cmake >= 3.1
endif()

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.3.0")
  #  Policy CMP0057 is not set: Support new IN_LIST if() operator.  Run "cmake
  #  --help-policy CMP0057" for policy details.  Use the cmake_policy command to
  #  set the policy and suppress this warning.
  #
  #  IN_LIST will be interpreted as an operator when the policy is set to NEW.
  #  Since the policy is not set the OLD behavior will be used.
  cmake_policy(SET CMP0057 NEW)
endif()

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.12.0")
  #  Policy CMP0074 is not set: find_package uses PackageName_ROOT variables.
  #  Run "cmake --help-policy CMP0074" for policy details.  Use the
  #  cmake_policy command to set the policy and suppress this warning.
  cmake_policy(SET CMP0074 NEW) # cmake >= 3.12
endif()

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.14.0")
  # Policy CMP0087 is not set: Install CODE|SCRIPT allow the use of generator
  # expressions.  Run "cmake --help-policy CMP0087" for policy details.  Use
  # the cmake_policy command to set the policy and suppress this warning.
  cmake_policy(SET CMP0087 NEW) # cmake >= 3.14
endif()

# enable project folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

###############################################################################
## cmake builtin search paths and includes ####################################
###############################################################################

LIST(APPEND CMAKE_MODULE_PATH "${BITTOOLS_CMAKE_ROOT}")

# cmake extension variables, must be before the first include
#set(TACKLELIB_ENABLE_TARGETS_EXTENSION_FUNCTION_HANDLERS 1) # 0 - macro handlers, faster; 1 - function handlers, slower

include(tacklelib/Project)
include(tacklelib/EnableTargetsExtension)
include(tacklelib/_3dparty/Qt)

###############################################################################
## builtin detection ##########################################################
###############################################################################

## CAUTION:
##   Must be called before the `project(...)` to workaround the error:
##   `CMake Error: CMake was unable to find a build program corresponding to "MinGW Makefiles".  CMAKE_MAKE_PROGRAM is not set.  You probably need to select a different build tool.`
##   by declare respective variable BEFORE the project main initialization.
##
#tkl_preload_variables(-S -s "CMAKE_MAKE_PROGRAM" "PATH;CMAKE_MAKE_PROGRAM")

# builtin detection runs by the `project` function
project(bittools)

set(BITSYNC_TARGET      bitsync)
set(XORFILE_TARGET      xorfile)
set(MIRRORFILE_TARGET   mirrorfile)
set(GENCRCTBL_TARGET    gencrctbl)

tkl_configure_environment(BITTOOLS_RUNTIME_LINK_TYPE "MSVC;GCC")

###############################################################################
## check environment variables ################################################
###############################################################################

tkl_check_var(REQUIRED . BITTOOLS_ADDRESS_MODEL)
tkl_check_var(REQUIRED . BITTOOLS_RUNTIME_LINK_TYPE)

# boost

tkl_check_var(REQUIRED PATH BOOST_ROOT)
tkl_check_var(REQUIRED . Boost_ARCHITECTURE) # required for boost 1.66 and higher: https://gitlab.kitware.com/cmake/cmake/merge_requests/2568

# tacklelib

tkl_check_var(REQUIRED PATH TACKLELIB_ROOT)

# fftw

#tkl_check_var(REQUIRED PATH FFTW_ROOT)

###############################################################################
## projects description #######################################################
###############################################################################

set(ALL_EXE_TARGETS ${BITSYNC_TARGET};${XORFILE_TARGET};${MIRRORFILE_TARGET};${GENCRCTBL_TARGET})

set(PRINT_FLAGS_TARGET_NAMES
  ${ALL_EXE_TARGETS}
)

# common optimization targets
set(COMMON_OPT_APP_TARGET_NAMES
  ${ALL_EXE_TARGETS}
)

###############################################################################
## file globbing ##############################################################
###############################################################################

# globbed public headers ONLY

file(GLOB_RECURSE all_public_headers
  ${CMAKE_CURRENT_LIST_DIR}/include/*.h*
)

file(GLOB_RECURSE all_private_headers
  ${CMAKE_CURRENT_LIST_DIR}/src/*.h*
)

file(GLOB_RECURSE all_cpp_sources
  ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
)

file(GLOB_RECURSE all_sources
  ${all_public_headers}
  ${all_private_headers}
  ${all_cpp_sources}
)

set(common_sources ${all_sources})
set(common_private_headers ${all_private_headers})

# exclude standalone sources
foreach(target ${ALL_EXE_TARGETS})
  tkl_exclude_paths_from_path_list(. common_sources "${common_sources}" "/src/${target}/" 0)
endforeach()

# exclude tests sources
tkl_exclude_paths_from_path_list(. common_sources "${common_sources}" "/tests/" 0)

# exclude by pattern
tkl_exclude_paths_from_path_list(. common_sources "${common_sources}" "/~" 0)

# exclude .bak files
tkl_exclude_paths_from_path_list(. common_sources "${common_sources}" ".bak" 0)

# generate standalone sources groups
foreach(target ${ALL_EXE_TARGETS})
  file(GLOB_RECURSE sources_${target} src/${target}/*.cpp src/${target}/*.h*)
  file(GLOB_RECURSE resources_${target} src/${target}/*.tpl src/${target}/*.def src/${target}/*.txt)

  # exclude target headers
  tkl_exclude_paths_from_path_list(. common_private_headers "${common_private_headers}" "${sources_${target}}" 0)

  # exclude tests sources
  tkl_exclude_paths_from_path_list(. sources_${target} "${sources_${target}}" "/tests/" 0)
  tkl_exclude_paths_from_path_list(. resources_${target} "${resources_${target}}" "/tests/" 0)
  tkl_exclude_paths_from_path_list(. common_private_headers "${common_private_headers}" "/tests/" 0)

  # exclude by pattern
  tkl_exclude_paths_from_path_list(. sources_${target} "${sources_${target}}" "/~" 0)
  tkl_exclude_paths_from_path_list(. resources_${target} "${resources_${target}}" "/~" 0)
  tkl_exclude_paths_from_path_list(. common_private_headers "${common_private_headers}" "/~" 0)

  # exclude .bak files
  tkl_exclude_paths_from_path_list(. sources_${target} "${sources_${target}}" ".bak" 0)
  tkl_exclude_paths_from_path_list(. resources_${target} "${resources_${target}}" ".bak" 0)
  tkl_exclude_paths_from_path_list(. common_private_headers "${common_private_headers}" ".bak" 0)
endforeach()

#file(GLOB_RECURSE headers_test src/tests/*.h*)

# exclude by pattern
tkl_exclude_paths_from_path_list(. headers_test "${headers_test}" "/~" 0)

# exclude .bak files
tkl_exclude_paths_from_path_list(. headers_test "${headers_test}" ".bak" 0)

#file(GLOB_RECURSE sources_test src/tests/*.cpp ${headers_test})
#
## exclude by pattern
#tkl_exclude_paths_from_path_list(. sources_test "${sources_test}" "/~" 0)
#
## exclude .bak files
#tkl_exclude_paths_from_path_list(. sources_test "${sources_test}" ".bak" 0)

# You can use set(sources src/main.cpp) etc if you don't want to
# use globbing to find files automatically.

###############################################################################
## find_package dependencies ##################################################
###############################################################################

# boost

find_package(BOOST_ROOT Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})

if (Boost_FOUND)
  message(STATUS "(*) Found `Boost`: `${Boost_VERSION}` Location: `${BOOST_ROOT}` Libs: [${Boost_LIBRARIES}]")
else()
  message(WARNING "(*) Boost is not found")
endif()

###############################################################################
## add_subdirectory dependencies ##############################################
###############################################################################

# tacklelib

tkl_add_target_subdirectory(TACKLELIB_ROOT tacklelib _3dparty/utility/tacklelib)

if (TARGET tacklelib)
  list(APPEND COMMON_OPT_APP_TARGET_NAMES tacklelib)

  tkl_set_target_property(TACKLELIB_ROOT * * "^tacklelib$$|^tacklelib.testlib$$" *  . EXCLUDE_FROM_DEFAULT_BUILD ON) # all except output libraries
  tkl_set_target_property(TACKLELIB_ROOT * * "^tacklelib$$|^tacklelib.testlib$$" *  . EXCLUDE_FROM_ALL ON) # all except output libraries
endif()

if (TARGET tacklelib.testlib)
  list(APPEND COMMON_OPT_APP_TARGET_NAMES tacklelib.testlib)
endif()

# fftw

tkl_add_target_subdirectory(FFTW_ROOT fftw _3dparty/math/fftw)

if (TARGET fftw)
  list(APPEND COMMON_OPT_APP_TARGET_NAMES fftw)

  tkl_set_target_property(FFTW_ROOT * * "^fftw$$" *  . EXCLUDE_FROM_DEFAULT_BUILD ON) # all except output libraries
  tkl_set_target_property(FFTW_ROOT * * "^fftw$$" *  . EXCLUDE_FROM_ALL ON) # all except output libraries
endif()

###############################################################################
## target definitions #########################################################
###############################################################################

# The data is just added to the executable, because in some IDEs (QtCreator) 
# files are invisible when they are not explicitly part of the project.
foreach(exe_target ${ALL_EXE_TARGETS})
  add_executable(${exe_target} ${common_sources} ${sources_${exe_target}} ${resources_${exe_target}})

  # This allows to include files relative to the root of the src directory with a <> pair
  target_include_directories(${exe_target}
    PUBLIC
      ${CMAKE_CURRENT_LIST_DIR}/src/_common
      ${CMAKE_CURRENT_LIST_DIR}/src
      ${CMAKE_CURRENT_LIST_DIR}/includes
      ${Boost_INCLUDE_DIRS}
  )

  target_include_directories(${exe_target}
    PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}
    PUBLIC
      ${CMAKE_CURRENT_LIST_DIR}/include
  )

  if (TACKLELIB_ROOT)
    target_include_directories(${exe_target}
      PRIVATE
        ${TACKLELIB_ROOT}
      PUBLIC
        ${TACKLELIB_ROOT}/include
    )
  endif()

  if (Boost_INCLUDE_DIRS)
    target_include_directories(${exe_target}
      PRIVATE
        ${Boost_INCLUDE_DIRS}
    )
  endif()

  if (FFTW_ROOT)
    target_include_directories(${exe_target}
      PRIVATE
        ${FFTW_ROOT}
      PUBLIC
        ${FFTW_ROOT}/api
    )
  endif()

  tkl_add_target_compile_definitions(${exe_target} *
    PRIVATE
      _UNICODE
  )

  # we need the same Boost definitions here to maintain the link with the same libraries
  if (Boost_FOUND)
    if (BOOST_ALL_NO_LIB)
      tkl_add_target_compile_definitions(${exe_target} *
        PUBLIC
          BOOST_ALL_NO_LIB
      )
    endif()

    if (BOOST_ALL_DYN_LINK)
      tkl_add_target_compile_definitions(${exe_target} *
        PUBLIC
          BOOST_ALL_DYN_LINK
      )
    endif()

    if (BOOST_SCOPE_EXIT_CONFIG_USE_LAMBDAS)
      tkl_add_target_compile_definitions(${exe_target} *
        PUBLIC
          BOOST_SCOPE_EXIT_CONFIG_USE_LAMBDAS # Force to use C++11 lambda functions to implement scope exits
      )
    endif()
  endif()

  if (WIN32)
    tkl_add_target_compile_definitions(${exe_target} *
      PRIVATE
        _X86_ # in case of dynamic library linkage which is required from Windows SDK: `fatal error C1189: #error:  "No Target Architecture"`
    )
  endif()

  if (Boost_LIBRARY_DIRS)
    tkl_add_target_link_directories(${exe_target} *
      PRIVATE
        ${Boost_LIBRARY_DIRS}
    )
  endif()

  if (TARGET tacklelib)
    target_link_libraries(${exe_target}
      PUBLIC
        tacklelib
    )
  endif()

  if (TARGET fftw3) # double, PREC_SUFFIX=<empty>
    target_link_libraries(${exe_target}
      PUBLIC
        fftw3
    )
  endif()
  if (TARGET fftw3f) # float, PREC_SUFFIX=f
    target_link_libraries(${exe_target}
      PUBLIC
        fftw3f
    )
  endif()

  if (Boost_LIBRARIES)
    target_link_libraries(${exe_target}
      PRIVATE
        ${Boost_LIBRARIES}
    )
  endif()

  if (WIN32)
    tkl_add_target_compile_definitions(${exe_target} *
      PRIVATE
        _X86_ # in case of dynamic library linkage which is required from Windows SDK: `fatal error C1189: #error:  "No Target Architecture"`
    )
  endif()

  tkl_initialize_executable_target_defaults(${exe_target} "${BITTOOLS_ADDRESS_MODEL}bit;console")

  tkl_source_groups_from_dir_list("Header Files (interface)" FILES ${CMAKE_CURRENT_LIST_DIR}/include *.h*)
  tkl_source_groups_from_dir_list("Header Files" FILES ${CMAKE_CURRENT_LIST_DIR}/src _common/*.h*)
  tkl_source_groups_from_dir_list("Header Files" FILES ${CMAKE_CURRENT_LIST_DIR}/src/${exe_target} *.h*)
  tkl_source_groups_from_dir_list("Source Files" FILES ${CMAKE_CURRENT_LIST_DIR}/src/${exe_target} *.c*)
  tkl_source_groups_from_dir_list("Resource Files" FILES ${CMAKE_CURRENT_LIST_DIR}/src/${exe_target} "*.tpl;*.txt")
endforeach()

###############################################################################
## custom targets #############################################################
###############################################################################

add_custom_command(
  TARGET
    ${BITSYNC_TARGET}
  PRE_BUILD
  COMMAND
    ${CMAKE_SHELL}
    /C @"${CMAKE_CURRENT_LIST_DIR}/BuildTools/TargetProject.PreBuildStep.bat"
    "${BITSYNC_TARGET}" "${CMAKE_CURRENT_LIST_DIR}/src/${BITSYNC_TARGET}" 1
  WORKING_DIRECTORY
    "${CMAKE_CURRENT_LIST_DIR}/src/${BITSYNC_TARGET}"
)

###############################################################################
## target optimization ########################################################
###############################################################################

# local optimization per target basis
if (MSVC)
  tkl_add_target_compile_properties("${COMMON_OPT_APP_TARGET_NAMES}" RELEASE
    /Ox     # Full Optimization
    /Ob2    # Inline Function Expansion: Any Suitable
    #/Oi     # Enable Intrinsic Functions
    /Ot     # Enable Intrinsic Functions
    /GL     # Whole Program Optimization
  )
  tkl_add_target_compile_properties("${COMMON_OPT_APP_TARGET_NAMES}" *
    /MP
  )

  tkl_add_target_link_properties("${COMMON_OPT_APP_TARGET_NAMES}" * RELEASE
    /LTCG   # Use Link Time Code Generation
  )
elseif (GCC)
  tkl_add_target_compile_properties("${COMMON_OPT_APP_TARGET_NAMES}" RELEASE
    -O3     # Full Optimization
    #/usr/bin/ranlib: .cpp.o: plugin needed to handle lto object
    #-flto   # Use Link Time Code Generation
  )
  tkl_add_target_compile_properties("${COMMON_OPT_APP_TARGET_NAMES}" *
    -pipe       # Use pipes rather than temporary files for communication between the various stages of compilation.
  )

  #/usr/bin/ranlib: .cpp.o: plugin needed to handle lto object
  #tkl_add_target_link_properties("${COMMON_OPT_APP_TARGET_NAMES}" NOTSTATIC RELEASE
  #  -flto   # Use Link Time Code Generation
  #)
  tkl_add_target_link_properties("${COMMON_OPT_APP_TARGET_NAMES}" NOTSTATIC *
    -pipe   # Use pipes rather than temporary files for communication between the various stages of compilation.
  )
endif()

###############################################################################
## target flags ###############################################################
###############################################################################

foreach(target_name IN LISTS PRINT_FLAGS_TARGET_NAMES)
  tkl_get_target_compile_property(COMPILE_OPTIONS_${target_name} ${target_name} .)
  tkl_get_target_link_property(LINK_FLAGS_${target_name} ${target_name} .)
  tkl_get_target_link_property(LINK_FLAGS_${target_name}_DEBUG ${target_name} DEBUG)
  tkl_get_target_link_property(LINK_FLAGS_${target_name}_RELEASE ${target_name} RELEASE)
  tkl_get_target_link_property(LINK_FLAGS_${target_name}_MINSIZEREL ${target_name} MINSIZEREL)
  tkl_get_target_link_property(LINK_FLAGS_${target_name}_RELWITHDEBINFO ${target_name} RELWITHDEBINFO)

  tkl_print_flags(
    COMPILE_OPTIONS_${target_name}
    LINK_FLAGS_${target_name}
    LINK_FLAGS_${target_name}_DEBUG
    LINK_FLAGS_${target_name}_RELEASE
    LINK_FLAGS_${target_name}_MINSIZEREL
    LINK_FLAGS_${target_name}_RELWITHDEBINFO
  )
endforeach()

###############################################################################
## packaging ##################################################################
###############################################################################

# All install commands get the same destination. this allows us to use paths
# relative to the executable.
install(
  TARGETS ${ALL_EXE_TARGETS}
  DESTINATION $<CONFIGURATION>
  COMPONENT exe)

# must be after all `install` commands!
#
set(install_code_dirs "")

if (BOOST_ROOT)
  list(APPEND install_code_dirs "${Boost_LIBRARY_DIRS}")
endif()

# must be after all `install` commands!
foreach(exe_target ${ALL_EXE_TARGETS})
  install(CODE "
    include(BundleUtilities)
    fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/\${CMAKE_INSTALL_CONFIG_NAME}/${exe_target}${CMAKE_EXECUTABLE_SUFFIX}\" \"\" \"${install_code_dirs}\")
    " COMPONENT ${exe_target})
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
    -P "${BITTOOLS_CMAKE_ROOT}/tacklelib/tools/CPackMakeConfig.cmake"
  COMMAND "${CMAKE_CPACK_COMMAND}" 
    -G "NSIS"
    -C "$<CONFIGURATION>"
    --config "${CPACK_OUTPUT_CONFIG_FILE}")

###############################################################################
## project folders ############################################################
###############################################################################

## projects

tkl_set_target_folder(CMAKE_CURRENT_LIST_DIR . * .       UTILITY     . util)
tkl_set_target_folder(CMAKE_CURRENT_LIST_DIR . * "tests" EXECUTABLE  . exe)
tkl_set_target_folder(CMAKE_CURRENT_LIST_DIR . * .       "SHARED_LIBRARY;STATIC_LIBRARY" . lib)

## 3dparty

# math

tkl_set_target_folder(FFTW_ROOT * * . * . _3dparty/math/fftw)

# utility

tkl_set_target_folder(BOOST_ROOT * * . * . _3dparty/utility/boost)

tkl_set_target_folder(TACKLELIB_ROOT * * .       UTILITY     . _3dparty/utility/tacklelib/util)
tkl_set_target_folder(TACKLELIB_ROOT * * "tests" EXECUTABLE  . _3dparty/utility/tacklelib/exe)
tkl_set_target_folder(TACKLELIB_ROOT * * .       "SHARED_LIBRARY;STATIC_LIBRARY" . _3dparty/utility/tacklelib/lib)
tkl_set_target_folder(TACKLELIB_ROOT * "tests" . * . _3dparty/utility/tacklelib/tests)
