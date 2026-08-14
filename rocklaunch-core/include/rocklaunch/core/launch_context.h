#pragma once

#include <filesystem>
#include <string>

namespace rocklaunch
{

namespace fs = std::filesystem;

// Everything the launch flow needs: where the game is, where the prefix lives, and which runner to use.
struct LaunchContext
{
    fs::path installDir;
    fs::path prefixDir;
    std::string runtimeId;
};

} // namespace rocklaunch
