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

# Runner set with a nonexistent profile must fail.
RunCli(1 runner set nonexistent-profile steam-proton-proton-test)
if (NOT LAST_OUTPUT MATCHES "Profile not found")
    message(FATAL_ERROR "Setting a runner on a nonexistent profile did not fail")
endif()

# Runner set with a nonexistent runner must fail.
RunCli(1 runner set test-profile nonexistent-runner)
if (NOT LAST_OUTPUT MATCHES "Runner not found")
    message(FATAL_ERROR "Setting a nonexistent runner did not fail")
endif()

# A profile without a runner assigned must show 'not assigned'.
RunCli(0 profile new no-runner)
RunCli(0 profile show no-runner)
if (NOT LAST_OUTPUT MATCHES "Runner: not assigned")
    message(FATAL_ERROR "Unassigned runner should show 'not assigned'")
endif()

# Reassigning a runner on an existing profile must work.
RunCli(0 runner set test-profile steam-proton-proton-test)
RunCli(0 profile show test-profile)
if (NOT LAST_OUTPUT MATCHES "Runner: steam-proton-proton-test")
    message(FATAL_ERROR "Runner reassignment did not take effect")
endif()

# Launcher runner source: create a runner under the launcher data dir.
file(MAKE_DIRECTORY "${TEST_ROOT}/data/rocksmith-launcher/runners/GE-Proton Test")
file(WRITE "${TEST_ROOT}/data/rocksmith-launcher/runners/GE-Proton Test/proton" "")
RunCli(0 runner list)
if (NOT LAST_OUTPUT MATCHES "launcher")
    message(FATAL_ERROR "Launcher runner source was not discovered")
endif()

# Profile remove must succeed with -f.
RunCli(0 profile remove -f no-runner)
RunCli(1 profile show no-runner)
if (NOT LAST_OUTPUT MATCHES "Profile not found")
    message(FATAL_ERROR "Deleted profile is still available")
endif()
