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
file(MAKE_DIRECTORY "${TEST_ROOT}/home/.steam/steam/compatibilitytools.d/GE-Proton Test")
file(MAKE_DIRECTORY "${TEST_ROOT}/games/rock/dlc")
file(WRITE "${TEST_ROOT}/games/rock/Rocksmith2014.exe" "")
file(WRITE "${TEST_ROOT}/home/.steam/steam/compatibilitytools.d/GE-Proton Test/proton" "")
file(CHMOD "${TEST_ROOT}/home/.steam/steam/compatibilitytools.d/GE-Proton Test/proton"
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)

# Creating a profile assigns its prefix path right away.
RunCli(0 profile new test-profile)
file(READ "${TEST_ROOT}/data/rocksmith-launcher/profiles/test-profile.json" profileJson)
if (NOT "${profileJson}" MATCHES "prefixes/test-profile")
    message(FATAL_ERROR "profile new did not persist the default prefix_dir")
endif()

# A profile without an install path must be rejected before launching.
RunCli(1 launch test-profile)
if (NOT LAST_OUTPUT MATCHES "set-path")
    message(FATAL_ERROR "Launching without an install path should mention set-path")
endif()

# A profile with a path but no runner must be rejected too.
RunCli(0 set-path test-profile "${TEST_ROOT}/games/rock")
RunCli(1 launch test-profile)
if (NOT LAST_OUTPUT MATCHES "runner set")
    message(FATAL_ERROR "Launching without a runner should mention runner set")
endif()

# With a runner the launch prepares the prefix and runs the game process.
RunCli(0 runner set test-profile steam-proton-ge-proton-test)
RunCli(0 launch test-profile)
if (NOT EXISTS "${TEST_ROOT}/data/rocksmith-launcher/prefixes/test-profile")
    message(FATAL_ERROR "The launch did not create the profile prefix")
endif()
