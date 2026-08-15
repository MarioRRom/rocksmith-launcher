cmake_minimum_required(VERSION 3.21)

project(RocksmithLauncher
    VERSION 0.1.0
    DESCRIPTION "A Linux launcher for Rocksmith 2014 Edition Remastered"
    LANGUAGES CXX
)

include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(nlohmann_json 3.11 QUIET)

if (NOT nlohmann_json_FOUND)
    include(FetchContent)

    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

add_subdirectory(rocklaunch-core)
add_subdirectory(rocklaunch-cli)

if (BUILD_TESTING)
    add_test(
        NAME rocklaunch-cli-help
        COMMAND rocklaunch-cli --help
    )
    set_tests_properties(rocklaunch-cli-help PROPERTIES
        ENVIRONMENT "XDG_DATA_HOME=${CMAKE_BINARY_DIR}/test-data"
    )

    add_test(
        NAME rocklaunch-cli-profile-lifecycle
        COMMAND ${CMAKE_COMMAND}
            -DROCKLAUNCH_CLI=$<TARGET_FILE:rocklaunch-cli>
            -DTEST_ROOT=${CMAKE_BINARY_DIR}/profile-lifecycle-test
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/profile_lifecycle.cmake
    )

    add_test(
        NAME rocklaunch-cli-runtime-manager
        COMMAND ${CMAKE_COMMAND}
            -DROCKLAUNCH_CLI=$<TARGET_FILE:rocklaunch-cli>
            -DTEST_ROOT=${CMAKE_BINARY_DIR}/runtime-manager-test
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/runtime_manager.cmake
    )

    add_test(
        NAME rocklaunch-cli-launch
        COMMAND ${CMAKE_COMMAND}
            -DROCKLAUNCH_CLI=$<TARGET_FILE:rocklaunch-cli>
            -DTEST_ROOT=${CMAKE_BINARY_DIR}/launch-test
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/launch.cmake
    )
endif()
