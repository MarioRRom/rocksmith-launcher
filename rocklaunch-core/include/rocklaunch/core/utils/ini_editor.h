#pragma once

#include <filesystem>
#include <string>

namespace rocklaunch
{

namespace fs = std::filesystem;

// Minimal INI file reader/writer. Creates file, section, or key when missing.
// Preserves comments and blank lines. On duplicate keys, returns the last value.
namespace IniEditor
{

std::string Get(const fs::path &file, const std::string &section,
                const std::string &key);

void Set(const fs::path &file, const std::string &section,
         const std::string &key, const std::string &value);

} // namespace IniEditor
} // namespace rocklaunch
