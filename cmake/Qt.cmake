function(detect_qt_creator)
  string(REGEX MATCH "[^a-zA-Z]?(QtCreator|qtc)[^a-zA-Z]?" QT_CREATOR_MATCH_STR "${CMAKE_CURRENT_BINARY_DIR}")
  if (QT_CREATOR_MATCH_STR)
    set(IS_EXECUTED_BY_QT_CREATOR 1 PARENT_SCOPE)
  endif()
endfunction()

# MUST BE A MACRO, OTHERWISE THE INNER find_package PARAMETERS WOULD STAY IN THE FUNCTION SCOPE!
macro(find_qt_component name is_required)
  if(${is_required})
    find_package(${name} REQUIRED)
  else()
    find_package(${name})
  endif()

  if(${name}_FOUND)
    list(APPEND QT_LIBRARIES ${${name}_LIBRARIES})
    list(APPEND QT_COMPONENTS ${name})
  endif()
endmacro()

function(link_qt_components target targets_list_var components_list_var)
  set(target_index 0)

  foreach(target_from_list ${${targets_list_var}})
    if(TARGET ${target_from_list})
      list(GET ${components_list_var} ${target_index} component)

      if(component)
        if(${component}_DEFINITIONS)
          add_definitions(
            ${${component}_DEFINITIONS})
          #message(STATUS "* ${component}_DEFINITIONS=${${component}_DEFINITIONS}")
        endif()

        if(${component}_INCLUDE_DIRS)
          target_include_directories(${target}
            PUBLIC
              ${${component}_INCLUDE_DIRS}
          )
          #message(STATUS "* ${component}_INCLUDE_DIRS=${${component}_INCLUDE_DIRS}")
        endif()

        if(${component}_LIBRARIES)
          target_link_libraries(${target}
            PUBLIC
              ${${component}_LIBRARIES}
          )
          #message(STATUS "* ${component}_LIBRARIES=${${component}_LIBRARIES}")
        endif()
      endif()

      target_link_libraries(${target}
        PUBLIC
          ${target_from_list}
      )
    endif()

    MATH(EXPR target_index "${target_index}+1")
  endforeach()
endfunction()

function(link_qt_libraries target targets_list)
  foreach(target_from_list ${targets_list})
      target_link_libraries(${target}
        PUBLIC
          ${target_from_list}
      )
  endforeach()
endfunction()
