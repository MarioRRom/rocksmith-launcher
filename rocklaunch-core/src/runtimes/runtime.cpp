#include "rocklaunch/core/runtimes/runtime.h"

#include <cctype>

namespace rocklaunch
{

std::string RuntimeTypeName(RuntimeType type)
{
    return type == RuntimeType::Wine ? "wine" : "proton";
}

std::string SanitizeRuntimeId(const std::string &name)
{
    std::string id;
    id.reserve(name.size());
    for (unsigned char character : name) {
        id += std::isalnum(character) ? static_cast<char>(std::tolower(character)) : '-';
    }
    return id;
}

} // namespace rocklaunch
