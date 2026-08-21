#pragma once

#include "rocklaunch/core/runners/runner_source.h"

namespace rocklaunch
{

class LauncherRunnerSource final : public IRunnerSource
{
public:
    explicit LauncherRunnerSource(fs::path runnerDir = DefaultRunnerDir());

    std::vector<Runner> Discover() const override;

    static fs::path DefaultRunnerDir();

private:
    fs::path m_runnerDir;
};

} // namespace rocklaunch
