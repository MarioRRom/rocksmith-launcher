#pragma once

#include "rocklaunch/core/runners/runner_source.h"

namespace rocklaunch
{

class SteamRunnerSource final : public IRunnerSource
{
public:
    explicit SteamRunnerSource(std::vector<fs::path> steamRoots = DefaultRoots());

    std::vector<Runner> Discover() const override;

    static std::vector<fs::path> DefaultRoots();

private:
    std::vector<fs::path> m_steamRoots;
};

} // namespace rocklaunch
