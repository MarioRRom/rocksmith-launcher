add_library(rocklaunch-core
    src/config_store.cpp
    src/launch.cpp
    src/logger.cpp
    src/manual_source.cpp
    src/rocksmith2014_profile.cpp
    src/runtimes/managed_runtime_source.cpp
    src/runtimes/runtime.cpp
    src/runtimes/runtime_manager.cpp
    src/runtimes/steam_runtime_source.cpp
    src/runtimes/system_wine_source.cpp
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
