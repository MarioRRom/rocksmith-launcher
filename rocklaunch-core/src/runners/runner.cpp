#include "rocklaunch/core/runners/runner.h"

#include <cctype>

namespace rocklaunch
{

std::string RunnerTypeName(RunnerType type)
{
    return type == RunnerType::Wine ? "wine" : "proton";
}

std::string SanitizeRunnerId(const std::string &name)
{
    std::string id;
    id.reserve(name.size());
    for (unsigned char character : name) {
        id += std::isalnum(character) ? static_cast<char>(std::tolower(character)) : '-';
    }
    return id;
}

} // namespace rocklaunch
