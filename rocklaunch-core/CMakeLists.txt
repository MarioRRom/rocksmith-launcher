add_library(rocklaunch-core
    src/config_store.cpp
    src/launch.cpp
    src/logger.cpp
    src/manual_source.cpp
    src/patches/cdlc_patch.cpp
    src/patches/direct_connect_patch.cpp
    src/patches/patch.cpp
    src/patches/patch_manager.cpp
    src/rocksmith2014_remastered_profile.cpp
    src/runners/managed_runner_source.cpp
    src/runners/runner.cpp
    src/runners/runner_manager.cpp
    src/runners/steam_runner_source.cpp
    src/runners/system_wine_source.cpp
)

add_library(rocklaunch::core ALIAS rocklaunch-core)

target_include_directories(rocklaunch-core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(rocklaunch-core
    PRIVATE
        nlohmann_json::nlohmann_json
)

target_compile_features(rocklaunch-core PUBLIC cxx_std_17)
