################################################################################################
# Univeral Arithmetic Library status report function.
# Automatically align right column and selects text based on condition.

function(universal_status text)
  set(status_cond)
  set(status_then)
  set(status_else)

  set(status_current_name "cond")
  foreach(arg ${ARGN})
    if(arg STREQUAL "THEN")
      set(status_current_name "then")
    elseif(arg STREQUAL "ELSE")
      set(status_current_name "else")
    else()
      list(APPEND status_${status_current_name} ${arg})
    endif()
  endforeach()

  if(DEFINED status_cond)
    set(status_placeholder_length 34)
    string(RANDOM LENGTH ${status_placeholder_length} ALPHABET " " status_placeholder)
    string(LENGTH "${text}" status_text_length)
    if(status_text_length LESS status_placeholder_length)
      string(SUBSTRING "${text}${status_placeholder}" 0 ${status_placeholder_length} status_text)
    elseif(DEFINED status_then OR DEFINED status_else)
      message(STATUS "${text}")
      set(status_text "${status_placeholder}")
    else()
      set(status_text "${text}")
    endif()

    if(DEFINED status_then OR DEFINED status_else)
      if(${status_cond})
        string(REPLACE ";" " " status_then "${status_then}")
        string(REGEX REPLACE "^[ \t]+" "" status_then "${status_then}")
        message(STATUS "${status_text} ${status_then}")
      else()
        string(REPLACE ";" " " status_else "${status_else}")
        string(REGEX REPLACE "^[ \t]+" "" status_else "${status_else}")
        message(STATUS "${status_text} ${status_else}")
      endif()
    else()
      string(REPLACE ";" " " status_cond "${status_cond}")
      string(REGEX REPLACE "^[ \t]+" "" status_cond "${status_cond}")
      message(STATUS "${status_text} ${status_cond}")
    endif()
  else()
    message(STATUS "${text}")
  endif()
endfunction()

################################################################################################
# Function merging lists of compiler flags to single string.
# Usage:
#   universal_merge_flag_lists(out_variable <list1> [<list2>] [<list3>] ...)
function(universal_merge_flag_lists out_var)
  set(__result "")
  foreach(__list ${ARGN})
    foreach(__flag ${${__list}})
      string(STRIP ${__flag} __flag)
      set(__result "${__result} ${__flag}")
    endforeach()
  endforeach()
  string(STRIP ${__result} __result)
  set(${out_var} ${__result} PARENT_SCOPE)
endfunction()

####
# Prints accumulated universal arithmetic library configuration summary
# Usage:
#   universal_print_configuration_summary()

function(universal_print_configuration_summary)

    universal_merge_flag_lists(__c_flags_rel CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS)
    universal_merge_flag_lists(__c_flags_deb CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS)
    universal_merge_flag_lists(__cxx_flags_rel CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS)
    universal_merge_flag_lists(__cxx_flags_deb CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS)

    universal_status("")
    universal_status("******************* Universal Arithmetic Library Configuration Summary *******************")
    universal_status("General:")
    universal_status("  Version                      :   ${PROJECT_VERSION}")
    universal_status("  System                       :   ${CMAKE_SYSTEM_NAME}")
    universal_status("  C compiler                   :   ${CMAKE_C_COMPILER}")
    universal_status("  Release C flags              :   ${__c_flags_rel}")
    universal_status("  Debug C flags                :   ${__c_flags_deb}")
    universal_status("  C++ compiler                 :   ${CMAKE_CXX_COMPILER}")
    universal_status("  Release CXX flags            :   ${__cxx_flags_rel}")
    universal_status("  Debug CXX flags              :   ${__cxx_flags_deb}")
    universal_status("  Build type                   :   ${CMAKE_BUILD_TYPE}")
    universal_status("")
    universal_status("  BUILD_CI_CHECK               :   ${BUILD_CI_CHECK}")
    universal_status("")
    universal_status("  BUILD_STORAGE_CLASSES        :   ${BUILD_STORAGE_CLASSES}")
    universal_status("  BUILD_NATIVE_TYPES           :   ${BUILD_NATIVE_TYPES}")
    universal_status("  BUILD_INTEGERS               :   ${BUILD_INTEGERS}")
    universal_status("  BUILD_DECIMALS               :   ${BUILD_DECIMALS}")
    universal_status("  BUILD_FIXPNTS                :   ${BUILD_FIXPNTS}")
    universal_status("  BUILD_LNS                    :   ${BUILD_REALS}")
    universal_status("  BUILD_UNUM_TYPE_1            :   ${BUILD_UNUM_TYPE_1}")
    universal_status("  BUILD_UNUM_TYPE_2            :   ${BUILD_UNUM_TYPE_2}")
    universal_status("  BUILD_POSITS                 :   ${BUILD_POSITS}")
    universal_status("  BUILD_VALIDS                 :   ${BUILD_VALIDS}")
    universal_status("  BUILD_REALS                  :   ${BUILD_REALS}")
    universal_status("")
    universal_status("  BUILD_C_API_PURE_LIB         :   ${BUILD_C_API_PURE_LIB}")
    universal_status("  BUILD_C_API_SHIM_LIB         :   ${BUILD_C_API_SHIM_LIB}")
    universal_status("  BUILD_C_API_LIB_PIC          :   ${BUILD_C_API_LIB_PIC}")
    universal_status("")
    universal_status("  BUILD_CMD_LINE_TOOLS         :   ${BUILD_CMD_LINE_TOOLS}")
    universal_status("  BUILD_EDUCATION              :   ${BUILD_EDUCATION}")
    universal_status("  BUILD_APPLICATIONS           :   ${BUILD_APPLICATIONS}")
    universal_status("  BUILD_NUMERICAL              :   ${BUILD_NUMERICAL}")
    universal_status("  BUILD_FUNCTIONS              :   ${BUILD_FUNCTIONS}")
    universal_status("  BUILD_PLAYGROUND             :   ${BUILD_PLAYGROUND}")
    universal_status("")
    universal_status("  BUILD_CONVERSION_TESTS       :   ${BUILD_CONVERSION_TESTS}")
    universal_status("")
    universal_status("  BUILD_PERFORMANCE_TESTS      :   ${BUILD_PERFORMANCE_TESTS}")
    universal_status("")
    universal_status("  BUILD_IEEE_FLOAT_QUIRES      :   ${BUILD_IEEE_FLOAT_QUIRES}")
    universal_status("")
    universal_status("  BUILD_DOCS                   :   ${BUILD_DOCS}")
    universal_status("")
    universal_status("Dependencies:")
    universal_status("  SSE3                         : " USE_SSE3 AND COMPILER_HAS_SSE3_FLAG THEN "YES" ELSE "NO")
    universal_status("  AVX                          : " USE_AVX AND COMPILER_HAS_AVX_FLAG THEN "YES" ELSE "NO")
    universal_status("  AVX2                         : " USE_AVX2 AND COMPILER_HAS_AVX2_FLAG THEN "YES" ELSE "NO")
    universal_status("  Pthread                      : " USE_PTHREAD THEN "YES" ELSE "NO")
    universal_status("  TBB                          : " USE_TBB AND TBB_FOUND THEN "YES (ver. ${TBB_INTERFACE_VERSION})" ELSE "NO")
    universal_status("  OMP                          : " USE_OMP AND OMP_FOUND THEN "YES" ELSE "NO")
    universal_status("")
    universal_status("Utilities:")
    universal_status("  Serializer                   : " USE_SERIALIZER THEN "YES" ELSE "NO")
    universal_status("")
    universal_status("Install:")
    universal_status("  Install path                 :   ${CMAKE_INSTALL_PREFIX}")
    universal_status("")
endfunction()
