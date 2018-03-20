# CAUTION:
# 1. Be careful with the `set(... CACHE ...)` because it removes the original variable!
#    From documation:
#     "Finally, whenever a cache variable is added or modified by a command,
#     CMake also removes the normal variable of the same name from the current
#     scope so that an immediately following evaluation of it will expose the
#     newly cached value."
# 2. Be careful with the `set(... CACHE ... FORCE)` because it overrides a value passed by
#    the `-D` cmake command line parameter!
#

macro(include_and_echo path)
  message(STATUS "(*) Include: \"${path}\"")
  include(${path})
endmacro()

function(cache_or_discover_variable var cache_type desc)
  if(NOT ${var} AND NOT "$ENV{${var}}" STREQUAL "")
    set(${var} $ENV{${var}} CACHE ${cache_type} ${desc}) # before the normal set, overwise it will remove the normal variable!
    set(${var} $ENV{${var}} PARENT_SCOPE)
  endif()
endfunction()

function(discover_variable var cache_type desc)
  # save cached value
  set(cached_var ${${var}})

  unset(${var} CACHE)

  set(uncached_var ${${var}})

  if(NOT uncached_var)
    if(NOT "$ENV{${var}}" STREQUAL "")
      set(${var} $ENV{${var}} CACHE ${cache_type} ${desc}) # before the normal set, overwise it will remove the normal variable!
      set(${var} $ENV{${var}} PARENT_SCOPE)
    endif()
  elseif(NOT "$ENV{${var}}" STREQUAL "")
    set(${var} $ENV{${var}} CACHE ${cache_type} ${desc}) # before the normal set, overwise it will remove the normal variable!
    # restore uncached variable removed by previous set with CACHE
    set(${var} ${uncached_var})
  else()
    # reset the cache to the normal value
    set(${var} ${uncached_var} CACHE ${cache_type} ${desc})
    # restore uncached variable removed by previous set with CACHE
    set(${var} ${uncached_var})
  endif()
endfunction()

macro(configure_environment)
  # CAUTION:
  #   We have to detect the executor to check if the `environment_local.cmake`
  #   has to be already generated. If not we must stop immediately and warn the
  #   user to run the `_script/configure_nogen` BEFORE cmake direct execution by
  #   the IDE!

  detect_qt_creator()
  if (IS_EXECUTED_BY_QT_CREATOR AND NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/environment_local.cmake)
    message(FATAL_ERROR "(*) The `environment_local.cmake` is not properly generated, use the `_scripts/configure_nogen` to generage the file and then edit values manually!")
  endif()

  if (NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/environment_local.cmake)
    # generates `environment_local.cmake` from the `environment_local.cmake.in` if not done yet and includes it unconditionally
    configure_file_and_include_required(${CMAKE_CURRENT_LIST_DIR}/environment_local.cmake.in ${CMAKE_CURRENT_LIST_DIR}/environment_local.cmake)
  endif()

  # include local environment
  include_and_echo(environment_local.cmake)

  # CAUTION:
  #   IDE like QtCreator uses `CMakeLists.txt.user` file to store and load cached
  #   versions of cmake environment variables. But it's change in cmake won't
  #   promote respective change to IDE. To make it changed you have to CLOSE IDE
  #   AND DELETE FILE WITH CACHED VARIABLES - `CMakeLists.txt.user`!

  discover_variable(ENV_ROOT PATH "environment root directory")
  discover_variable(ENV_FILENAME STRING "environment file name")

  # searches `environment.cmake` and included it, basically this environment contains global environment additional to the local environment
  include(FindEnvironment)

  # in case of intersection reinclude the local environment
  include_and_echo(environment_local.cmake)
endmacro()

function(configure_file_and_include_impl tmpl_file_path out_file_path is_required)
  if(NOT EXISTS ${tmpl_file_path})
    message(FATAL_ERROR "template input file does not exist: \"${tmpl_file_path}\"")
  endif()

  get_filename_component(out_file_dir ${out_file_path} DIRECTORY)
  if(NOT EXISTS ${out_file_dir})
    message(FATAL_ERROR "output file directory does not exist: \"${out_file_dir}\"")
  endif()

  # override current environment variables by locally stored
  if(is_required OR NOT EXISTS "${out_file_path}")
    set(CONFIGURE_IN_FILE "${tmpl_file_path}")
    set(CONFIGURE_OUT_FILE "${out_file_path}")
    include(ConfigureFile)
  endif()

  if(EXISTS "${out_file_path}")
    include("${out_file_path}")
    message(STATUS "(*) Configurated files: \"${tmpl_file_path}\" -> \"${out_file_path}\"")
  endif()
endfunction()

function(configure_file_and_include_optional tmpl_file_path out_file_path)
  configure_file_and_include_impl(${tmpl_file_path} ${out_file_path} 0)
endfunction()

function(configure_file_and_include_required tmpl_file_path out_file_path)
  configure_file_and_include_impl(${tmpl_file_path} ${out_file_path} 1)
endfunction()

function(exclude_paths_from_path_list include_list_var path_list exclude_path_list verbose_flag)
  if(verbose_flag)
    message(STATUS "(**) exclude_paths_from_path_list: exclude list: ${exclude_path_list}")
  endif()

  set(include_list "")

  foreach(path ${path_list})
    set(_excluded 0)
    foreach(exclude_path ${exclude_path_list})
      if("${path}" MATCHES "(.*)${exclude_path}(.*)")
        if(verbose_flag)
          message(STATUS "(**) exclude_paths_from_path_list: excluded: ${path}")
        endif()
        set(_excluded 1)
        break()
      endif()
    endforeach()
    if(NOT _excluded)
      list(APPEND include_list "${path}")
    endif()
  endforeach()

  if(verbose_flag)
    message(STATUS "(**) exclude_paths_from_path_list: include list: ${include_list}")
  endif()

  set(${include_list_var} ${include_list} PARENT_SCOPE)
endfunction()

function(exclude_file_paths_from_path_list include_list_var path_list exclude_file_path_list verbose_flag)
  if(verbose_flag)
    message(STATUS "(**) exclude_file_paths_from_path_list: exclude list: ${exclude_file_path_list}")
  endif()

  set(include_list "")

  foreach(path ${path_list})
    set(_excluded 0)
    foreach(exclude_file_path ${exclude_file_path_list})
      if("${path}|" MATCHES "(.*)${exclude_file_path}\\|")
        if(verbose_flag)
          message(STATUS "(**) exclude_file_paths_from_path_list: excluded: ${path}")
        endif()
        set(_excluded 1)
        break()
      endif()
    endforeach()
    if(NOT _excluded)
      list(APPEND include_list "${path}")
    endif()
  endforeach()

  if(verbose_flag)
    message(STATUS "(**) exclude_file_paths_from_path_list: include list: ${include_list}")
  endif()

  set(${include_list_var} ${include_list} PARENT_SCOPE)
endfunction()

function(include_paths_from_path_list include_list_var path_list include_path_list verbose_flag)
  if(verbose_flag)
    message(STATUS "(**) include_paths_from_path_list: include list: ${include_path_list}")
  endif()

  set(include_list "")

  foreach(path ${path_list})
    foreach(include_path ${include_path_list})
      if("${path}" MATCHES "(.*)${include_path}(.*)")
        if(verbose_flag)
          message(STATUS "(**) include_paths_from_path_list: included: ${path}")
        endif()
        list(APPEND include_list "${path}")
      endif()
    endforeach()
  endforeach()

  set(${include_list_var} ${include_list} PARENT_SCOPE)
endfunction()

function(include_file_paths_from_path_list include_list_var path_list include_file_path_list verbose_flag)
  if(verbose_flag)
    message(STATUS "(**) include_file_paths_from_path_list: include list: ${include_file_path_list}")
  endif()

  set(include_list "")

  foreach(path ${path_list})
    foreach(include_file_path ${include_file_path_list})
      if("${path}|" MATCHES "(.*)${include_file_path}\\|")
        if(verbose_flag)
          message(STATUS "(**) include_file_paths_from_path_list: included: ${path}")
        endif()
        list(APPEND include_list "${path}")
      endif()
    endforeach()
  endforeach()

  set(${include_list_var} ${include_list} PARENT_SCOPE)
endfunction()

function(source_group_by_path_list group_path type path_list include_path_list verbose_flag)
  set(include_list "")

  foreach(path ${path_list})
    foreach(include_path ${include_path_list})
      if("${path}" MATCHES "(.*)${include_path}(.*)")
        if(verbose_flag)
          message(STATUS "(**) source_group_from_include_list: ${group_path} -> (${type}) \"${path}\"")
        endif()
        list(APPEND include_list ${path})
      endif()
    endforeach()
  endforeach()

  if(include_list)
    source_group("${group_path}" ${type} ${include_list})
  endif()
endfunction()

function(source_group_by_file_path_list group_path type path_list include_file_path_list verbose_flag)
  set(include_list "")

  foreach(path ${path_list})
    foreach(include_file_path ${include_file_path_list})
      if("${path}|" MATCHES "(.*)${include_file_path}\\|")
        if(verbose_flag)
          message(STATUS "(**) source_group_from_include_list: ${group_path} -> (${type}) \"${path}\"")
        endif()
        list(APPEND include_list ${path})
      endif()
    endforeach()
  endforeach()

  if(include_list)
    source_group("${group_path}" ${type} ${include_list})
  endif()
endfunction()

function(source_groups_from_dir_list source_group_root type path_dir_list path_glob_suffix)
  string(REGEX REPLACE "/" "\\\\" source_group_root "${source_group_root}")

  foreach(path_dir ${path_dir_list})
    #message(STATUS path_dir=${path_dir})
    if(NOT EXISTS ${path_dir}/)
      continue()
    endif()

    file(GLOB_RECURSE children_list RELATIVE ${path_dir} "${path_dir}/${path_glob_suffix}")

    set(group_path_dir_list "")

    get_filename_component(abs_path_dir ${path_dir} ABSOLUTE)

    foreach(child_path ${children_list})
      get_filename_component(abs_child_path ${path_dir}/${child_path} ABSOLUTE)

      file(RELATIVE_PATH child_rel_path ${abs_path_dir} ${abs_child_path})
      if(child_rel_path)
        get_filename_component(child_rel_dir ${child_rel_path} DIRECTORY)

        string(REGEX REPLACE "/" "\\\\" source_group_dir "${child_rel_dir}")
        if(source_group_root)
          #message(STATUS "source_groups_from_dir_list: ${source_group_root}\\${source_group_dir} -> ${child_rel_path}")
          source_group("${source_group_root}\\${source_group_dir}" ${type} "${path_dir}/${child_path}")
        else()
          #message(STATUS "source_groups_from_dir_list: ${source_group_dir} -> ${child_rel_path}")
          source_group("${source_group_dir}" ${type} "${path_dir}/${child_path}")
        endif()
      endif()
    endforeach()
  endforeach()
endfunction()

function(add_pch_header create_pch_header from_pch_src to_pch_bin use_pch_header include_pch_header sources sources_out_var)
  # MSVC arguments can be mixed, canonicalize all
  set(create_pch_header_fixed ${create_pch_header})
  set(from_pch_src_fixed ${from_pch_src})
  set(to_pch_bin_fixed ${to_pch_bin})
  set(use_pch_header_fixed ${use_pch_header})
  set(include_pch_header_fixed ${include_pch_header})
  set(sources_fixed "")

  string(REPLACE "\\" "/" create_pch_header_fixed ${create_pch_header_fixed})
  string(REPLACE "\\" "/" from_pch_src_fixed ${from_pch_src_fixed})
  string(REPLACE "\\" "/" to_pch_bin_fixed ${to_pch_bin_fixed})
  string(REPLACE "\\" "/" use_pch_header_fixed ${use_pch_header_fixed})
  string(REPLACE "\\" "/" include_pch_header_fixed ${include_pch_header_fixed})
  foreach(src ${sources})
    string(REPLACE "\\" "/" src_fixed ${src})
    list(APPEND sources_fixed ${src_fixed})
  endforeach()

  set(pch_bin_file "${CMAKE_CURRENT_BINARY_DIR}/${to_pch_bin_fixed}")

  exclude_file_paths_from_path_list(sources_filtered "${sources_fixed}" "/.*\\.h.*" 0)

  string(REPLACE "." "\\." from_pch_src_regex ${from_pch_src})
  exclude_file_paths_from_path_list(sources_filtered "${sources_filtered}" "/${from_pch_src_regex}" 0)

  set(use_and_include_pch_header "/Yu\"${use_pch_header_fixed}\"")
  if (include_pch_header)
    set(use_and_include_pch_header "${use_and_include_pch_header} /FI\"${include_pch_header_fixed}\"")
  endif()

  set_source_files_properties(${sources_filtered}
                              PROPERTIES COMPILE_FLAGS "${use_and_include_pch_header} /Fp\"${pch_bin_file}\""
                                         OBJECT_DEPENDS "${pch_bin_file}")  

  # at the last to reset the properties in case if `from_pch_src` is a part of `sources`
  set_source_files_properties(${from_pch_src_fixed}
                              PROPERTIES COMPILE_FLAGS "/Yc\"${create_pch_header_fixed}\" /Fp\"${pch_bin_file}\""
                                         OBJECT_OUTPUTS "${pch_bin_file}")

  if (sources_out_var)
    list(APPEND ${sources_out_var} ${pch_src})
    set(${sources_out_var} ${${sources_out_var}} PARENT_SCOPE)
  endif()
endfunction()

function(remove_global_optimization_flags)
  if(WIN32 AND MSVC)
    foreach(config_name ${ARGN})
      if(${config_name} STREQUAL "*")
        set(config_name_suffix "")
      else()
        set(config_name_suffix "_${config_name}")
      endif()
      foreach(flag "/O[^ \t]+" "/GL" "/GT")
        string(REGEX REPLACE " *${flag}|${flag} *" "" CMAKE_CXX_FLAGS${config_name_suffix} ${CMAKE_CXX_FLAGS${config_name_suffix}})
        set(CMAKE_CXX_FLAGS${config_name_suffix} ${CMAKE_CXX_FLAGS${config_name_suffix}} PARENT_SCOPE)
      endforeach()
      foreach(flag "/LTCG[^ \t]*")
        foreach(flag_var
          CMAKE_EXE_LINKER_FLAGS CMAKE_MODULE_LINKER_FLAGS CMAKE_STATIC_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS)
          string(REGEX REPLACE " *${flag}|${flag} *" "" ${flag_var}${config_name_suffix} "${${flag_var}${config_name_suffix}}")
          set(${flag_var}${config_name_suffix} ${${flag_var}${config_name_suffix}} PARENT_SCOPE)
        endforeach()
      endforeach()
    endforeach()
  else()
    message(FATAL_ERROR "platform is not implemented")
  endif()
endfunction()

function(fix_global_flags)
  # invalid case values
  if(MSVC)
    foreach(flag "/machine:X86")
      string(TOUPPER "${flag}" flag_uppercase)
      foreach(flag_var
        CMAKE_EXE_LINKER_FLAGS CMAKE_MODULE_LINKER_FLAGS CMAKE_STATIC_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS)
        string(REGEX REPLACE "${flag}" "${flag_uppercase}" ${flag_var} "${${flag_var}}")
        set(${flag_var} ${${flag_var}} PARENT_SCOPE)
      endforeach()
    endforeach()
  endif()
endfunction()

function(set_global_link_type type)
  if(${type} STREQUAL "dynamic")
    foreach(flag_var
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
      if(${flag_var} MATCHES "/MT")
        string(REGEX REPLACE "/MT" "/MD" ${flag_var} "${${flag_var}}")
        set(${flag_var} ${${flag_var}} PARENT_SCOPE)
      endif()
    endforeach()
  elseif (${type} STREQUAL "static")
    foreach(flag_var
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
      if(${flag_var} MATCHES "/MD")
        string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        set(${flag_var} ${${flag_var}} PARENT_SCOPE)
      endif()
    endforeach()
  endif()
endfunction()

function(add_target_compile_definitions targets type)
  foreach(target ${targets})
    if (ARGN)
      foreach(arg ${ARGN})
        # arg must string here
        target_compile_definitions(${target} ${type} "${arg}")
      endforeach()
    endif()
  endforeach()
endfunction()

function(add_target_compile_properties targets)
  foreach(target ${targets})
    get_target_property(PROP_LIST_${target} ${target} COMPILE_OPTIONS)

    # PROP_LIST can be list here
    set(PROP_LIST "")
    if (PROP_LIST_${target})
      foreach(arg ${PROP_LIST_${target}})
        list(APPEND PROP_LIST ${arg})
      endforeach()
    endif()
    if (ARGN)
      foreach(arg ${ARGN})
        list(APPEND PROP_LIST ${arg})
      endforeach()
    endif()

    if (PROP_LIST)
      set_target_properties(${target} PROPERTIES
        COMPILE_OPTIONS "${PROP_LIST}"
      )
    endif()
  endforeach()
endfunction()

function(add_target_link_properties targets configs)
  foreach(target ${targets})
    foreach(config ${configs})
      get_target_property(PROP_LIST_${target} ${target} LINK_FLAGS_${config})

      # PROP_LIST must be string here
      set(PROP_LIST "")
      if (PROP_LIST_${target})
        foreach(arg ${PROP_LIST_${target}})
          if (PROP_LIST)
            set(PROP_LIST "${PROP_LIST} ${arg}")
          else()
            set(PROP_LIST "${arg}")
          endif()
        endforeach()
      endif()
      if (ARGN)
        foreach(arg ${ARGN})
          if (PROP_LIST)
            set(PROP_LIST "${PROP_LIST} ${arg}")
          else()
            set(PROP_LIST "${arg}")
          endif()
        endforeach()
      endif()

      if (PROP_LIST)
        set_target_properties(${target} PROPERTIES
          LINK_FLAGS_${config} "${PROP_LIST}"
        )
      endif()
    endforeach()
  endforeach()
endfunction()

function(print_flags)
  if (ARGN)
    foreach(flag_var ${ARGN})
      if (${flag_var})
        message(STATUS "* ${flag_var}=${${flag_var}}")
      else()
        message(STATUS "* ${flag_var}=")
      endif()
    endforeach(flag_var)
  else()
    message(FATAL_ERROR "ARGN must be defined and not empty")
  endif()
endfunction()

function(print_global_flags)
  print_flags(
    CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
    CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
    CMAKE_EXE_LINKER_FLAGS CMAKE_EXE_LINKER_FLAGS_DEBUG CMAKE_EXE_LINKER_FLAGS_RELEASE
    CMAKE_EXE_LINKER_FLAGS_MINSIZEREL CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO
    CMAKE_MODULE_LINKER_FLAGS CMAKE_MODULE_LINKER_FLAGS_DEBUG CMAKE_MODULE_LINKER_FLAGS_RELEASE
    CMAKE_MODULE_LINKER_FLAGS_MINSIZEREL CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO
    CMAKE_STATIC_LINKER_FLAGS CMAKE_STATIC_LINKER_FLAGS_DEBUG CMAKE_STATIC_LINKER_FLAGS_RELEASE
    CMAKE_STATIC_LINKER_FLAGS_MINSIZEREL CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO
    CMAKE_SHARED_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS_DEBUG CMAKE_SHARED_LINKER_FLAGS_RELEASE
    CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO)
endfunction()
