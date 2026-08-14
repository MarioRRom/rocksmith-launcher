if (NOT DEFINED ROCKLAUNCH_CLI OR NOT DEFINED TEST_ROOT)
    message(FATAL_ERROR "ROCKLAUNCH_CLI and TEST_ROOT are required")
endif()

function(RunCli expectedResult)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env
            "XDG_CONFIG_HOME=${TEST_ROOT}/config"
            "XDG_DATA_HOME=${TEST_ROOT}/data"
            "${ROCKLAUNCH_CLI}" ${ARGN}
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
    )

    if (NOT "${result}" STREQUAL "${expectedResult}")
        message(FATAL_ERROR
            "Command '${ARGN}' returned ${result}; expected ${expectedResult}.\n"
            "Output: ${output}\nError: ${error}"
        )
    endif()

    set(LAST_OUTPUT "${output}${error}" PARENT_SCOPE)
endfunction()

file(REMOVE_RECURSE "${TEST_ROOT}")
file(MAKE_DIRECTORY "${TEST_ROOT}/games/steam/dlc")
file(MAKE_DIRECTORY "${TEST_ROOT}/games/external/dlc")
file(WRITE "${TEST_ROOT}/games/steam/Rocksmith2014.exe" "")
file(WRITE "${TEST_ROOT}/games/external/Rocksmith2014.exe" "")

RunCli(0 profile new steam)
RunCli(0 set-path steam "${TEST_ROOT}/games/steam")
RunCli(0 profile new external)
RunCli(0 set-path external "${TEST_ROOT}/games/external")
RunCli(0 profile new duplicate)
RunCli(1 set-path duplicate "${TEST_ROOT}/games/steam")

if (NOT LAST_OUTPUT MATCHES "already used by profile: steam")
    message(FATAL_ERROR "Duplicate installation path was not rejected as expected")
endif()

RunCli(0 profile delete steam)
RunCli(1 profile show steam)

if (NOT LAST_OUTPUT MATCHES "Profile not found: steam")
    message(FATAL_ERROR "Deleted profile is still available")
endif()
