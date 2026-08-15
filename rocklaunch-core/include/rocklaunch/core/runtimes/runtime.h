#pragma once

#include <filesystem>
#include <string>

namespace rocklaunch
{

namespace fs = std::filesystem;

enum class RuntimeType
{
    Wine,
    Proton,
};

struct Runtime
{
    std::string id;
    std::string name;
    RuntimeType type;
    std::string source;
    fs::path rootDir;
    fs::path executable;
};

std::string RuntimeTypeName(RuntimeType type);
std::string SanitizeRuntimeId(const std::string &name);

} // namespace rocklaunch
