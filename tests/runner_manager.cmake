if (NOT DEFINED ROCKLAUNCH_CLI OR NOT DEFINED TEST_ROOT)
    message(FATAL_ERROR "ROCKLAUNCH_CLI and TEST_ROOT are required")
endif()

function(RunCli expectedResult)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env
            "HOME=${TEST_ROOT}/home"
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
file(MAKE_DIRECTORY "${TEST_ROOT}/home/.steam/steam/compatibilitytools.d/Proton Test")
file(WRITE "${TEST_ROOT}/home/.steam/steam/compatibilitytools.d/Proton Test/proton" "")

RunCli(0 runner list)
if (NOT LAST_OUTPUT MATCHES "steam-proton-proton-test")
    message(FATAL_ERROR "The Steam Proton runner was not discovered")
endif()

RunCli(0 profile new test-profile)
RunCli(0 runner set test-profile steam-proton-proton-test)
RunCli(0 profile show test-profile)
if (NOT LAST_OUTPUT MATCHES "Runner: steam-proton-proton-test")
    message(FATAL_ERROR "The selected runner was not saved to the profile")
endif()
