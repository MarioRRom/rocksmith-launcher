#pragma once

#include <filesystem>
#include <string>

namespace rocklaunch
{

namespace fs = std::filesystem;

enum class RunnerType
{
    Wine,
    Proton,
};

struct Runner
{
    std::string id;
    std::string name;
    RunnerType type;
    std::string source;
    fs::path rootDir;
    fs::path executable;
};

std::string RunnerTypeName(RunnerType type);
std::string SanitizeRunnerId(const std::string &name);

} // namespace rocklaunch
