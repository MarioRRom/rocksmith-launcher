#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace rocklaunch
{

namespace fs = std::filesystem;

// Throws on non-zero exit code. When workDir is non-empty, the subprocess
// runs in that directory.
void RunSubprocess(const std::vector<std::string> &args,
                   const fs::path &workDir = {});

} // namespace rocklaunch
