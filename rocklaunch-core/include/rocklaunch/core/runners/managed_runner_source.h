#pragma once

#include "rocklaunch/core/runners/runner_source.h"

namespace rocklaunch
{

class ManagedRunnerSource final : public IRunnerSource
{
public:
    explicit ManagedRunnerSource(fs::path runnerDir = DefaultRunnerDir());

    std::vector<Runner> Discover() const override;

    static fs::path DefaultRunnerDir();

private:
    fs::path m_runnerDir;
};

} // namespace rocklaunch
