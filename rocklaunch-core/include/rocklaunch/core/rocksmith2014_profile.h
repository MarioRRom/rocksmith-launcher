#pragma once

#include "rocklaunch/core/game_profile.h"

namespace rocklaunch
{

class Rocksmith2014Profile final : public IGameProfile
{
public:
    std::string Id() const override;
    bool ValidateInstall(const fs::path &installDir) const override;
    std::vector<std::string> RequiredEnv(const LaunchContext &context) const override;
    fs::path Executable(const fs::path &installDir) const override;
};

} // namespace rocklaunch
