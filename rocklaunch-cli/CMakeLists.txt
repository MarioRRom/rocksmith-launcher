add_executable(rocklaunch-cli
    src/main.cpp
    src/cli_ui.cpp
)

target_link_libraries(rocklaunch-cli PRIVATE rocklaunch::core)

target_compile_features(rocklaunch-cli PRIVATE cxx_std_17)

target_compile_definitions(rocklaunch-cli PRIVATE
    ROCKLAUNCH_VERSION="${PROJECT_VERSION}"
)
