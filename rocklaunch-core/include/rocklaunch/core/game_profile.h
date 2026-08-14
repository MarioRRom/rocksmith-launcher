#pragma once

#include "rocklaunch/core/launch_context.h"

#include <string>
#include <vector>

namespace rocklaunch
{

// Behavior of a supported game. Does not represent a user installation — see ProfileConfig.
class IGameProfile
{
public:
    virtual ~IGameProfile() = default;

    virtual std::string Id() const = 0;
    // True when the expected binaries and folders are present in installDir.
    virtual bool ValidateInstall(const fs::path &installDir) const = 0;
    // Environment variables to set for this game under the given context.
    virtual std::vector<std::string> RequiredEnv(const LaunchContext &context) const = 0;
    // Path of the game binary inside installDir.
    virtual fs::path Executable(const fs::path &installDir) const = 0;
};

} // namespace rocklaunch
