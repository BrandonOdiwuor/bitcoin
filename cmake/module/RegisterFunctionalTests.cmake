# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.
#
# One CTest test per test_runner.py ALL_SCRIPTS entry.
# create_cache runs first (FIXTURES_SETUP), same contract as test_runner.py.

function(register_functional_tests)
  if(NOT BUILD_FUNCTIONAL_TESTS)
    return()
  endif()

  if(NOT Python3_EXECUTABLE)
    find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
  endif()

  set(_runner "${PROJECT_SOURCE_DIR}/test/functional/test_runner.py")
  execute_process(
    COMMAND ${Python3_EXECUTABLE} "${_runner}" --ctest-list
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/test/functional"
    OUTPUT_VARIABLE _list
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _list_result
  )
  if(NOT _list_result EQUAL 0 OR _list STREQUAL "")
    message(FATAL_ERROR
      "test_runner.py --ctest-list failed (result=${_list_result})")
  endif()

  set(_func_dir "${PROJECT_BINARY_DIR}/test/functional")
  set(_tmp_root "${PROJECT_BINARY_DIR}/test/tmp")
  set(_config   "${PROJECT_BINARY_DIR}/test/config.ini")
  set(_cache    "${PROJECT_BINARY_DIR}/test/cache")
  set(_cache_tmp "${_tmp_root}/functional.create_cache")

  add_test(
    NAME functional.create_cache
    COMMAND /bin/sh -c
            "rm -rf \"${_cache}\" \"${_cache_tmp}\" && exec \"${Python3_EXECUTABLE}\" \"${_func_dir}/create_cache.py\" --configfile=\"${_config}\" --cachedir=\"${_cache}\" --tmpdir=\"${_cache_tmp}\" --portseed=0"
  )
  set_tests_properties(functional.create_cache PROPERTIES
    LABELS "functional;setup"
    FIXTURES_SETUP FunctionalCache
    TIMEOUT 120
    COST 1
    PROCESSORS 1
    REQUIRED_FILES "${_config};${_func_dir}/create_cache.py"
    WORKING_DIRECTORY "${_func_dir}"
  )

  set(_portseed 1)
  string(REPLACE "\n" ";" _lines "${_list}")
  foreach(_line IN LISTS _lines)
    if(_line STREQUAL "")
      continue()
    endif()
    string(REPLACE "\t" ";" _fields "${_line}")
    list(LENGTH _fields _nfields)
    if(_nfields LESS 2)
      continue()
    endif()
    list(GET _fields 0 _kind)
    list(GET _fields 1 _spec)

    separate_arguments(_spec_args UNIX_COMMAND "${_spec}")
    list(GET _spec_args 0 _script)
    list(REMOVE_AT _spec_args 0)

    get_filename_component(_base "${_script}" NAME_WE)
    if(_base STREQUAL "create_cache")
      continue()
    endif()

    set(_tname "functional.${_base}")
    foreach(_arg IN LISTS _spec_args)
      string(REGEX REPLACE "^--" "" _arg_id "${_arg}")
      string(MAKE_C_IDENTIFIER "${_arg_id}" _arg_id)
      string(APPEND _tname ".${_arg_id}")
    endforeach()

    string(REGEX REPLACE "_.*" "" _family "${_base}")

    set(_timeout 600)
    set(_cost 10)
    set(_labels "functional;${_family}")
    if(_kind STREQUAL "extended")
      set(_timeout 1200)
      set(_cost 100)
      list(APPEND _labels "extended")
    endif()

    set(_tmpdir "${_tmp_root}/${_tname}")
    add_test(
      NAME ${_tname}
      COMMAND /bin/sh -c
              "rm -rf \"${_tmpdir}\" && exec \"${Python3_EXECUTABLE}\" \"${_func_dir}/${_script}\" ${_spec_args} --configfile=\"${_config}\" --cachedir=\"${_cache}\" --tmpdir=\"${_tmpdir}\" --portseed=${_portseed}"
    )
    set_tests_properties(${_tname} PROPERTIES
      LABELS "${_labels}"
      TIMEOUT ${_timeout}
      COST ${_cost}
      PROCESSORS 1
      SKIP_RETURN_CODE 77
      FIXTURES_REQUIRED FunctionalCache
      REQUIRED_FILES "${_config};${_func_dir}/${_script}"
      WORKING_DIRECTORY "${_func_dir}"
    )
    math(EXPR _portseed "${_portseed} + 1")
  endforeach()
endfunction()