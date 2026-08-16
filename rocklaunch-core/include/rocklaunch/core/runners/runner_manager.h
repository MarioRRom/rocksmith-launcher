#pragma once

#include "rocklaunch/core/runners/runner_source.h"

#include <memory>
#include <optional>
#include <vector>

namespace rocklaunch
{

class RunnerManager
{
public:
    explicit RunnerManager(std::vector<std::unique_ptr<IRunnerSource>> sources);

    std::vector<Runner> List() const;
    std::optional<Runner> Find(const std::string &runnerId) const;

    static RunnerManager CreateDefault();

private:
    std::vector<std::unique_ptr<IRunnerSource>> m_sources;
};

} // namespace rocklaunch
