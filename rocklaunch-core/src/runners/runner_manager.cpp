#include "rocklaunch/core/runners/runner_manager.h"

#include "rocklaunch/core/runners/managed_runner_source.h"
#include "rocklaunch/core/runners/steam_runner_source.h"
#include "rocklaunch/core/runners/system_wine_source.h"

#include <algorithm>
#include <set>

namespace rocklaunch
{

RunnerManager::RunnerManager(std::vector<std::unique_ptr<IRunnerSource>> sources)
    : m_sources(std::move(sources))
{
}

std::vector<Runner> RunnerManager::List() const
{
    std::vector<Runner> runners;
    std::set<std::string> ids;
    for (const std::unique_ptr<IRunnerSource> &source : m_sources) {
        for (Runner runner : source->Discover()) {
            if (ids.insert(runner.id).second) {
                runners.emplace_back(std::move(runner));
            }
        }
    }

    std::sort(runners.begin(), runners.end(), [](const Runner &left, const Runner &right) {
        return left.id < right.id;
    });
    return runners;
}

std::optional<Runner> RunnerManager::Find(const std::string &runnerId) const
{
    for (Runner runner : List()) {
        if (runner.id == runnerId) {
            return runner;
        }
    }

    return std::nullopt;
}

RunnerManager RunnerManager::CreateDefault()
{
    std::vector<std::unique_ptr<IRunnerSource>> sources;
    sources.emplace_back(std::make_unique<SystemWineSource>());
    sources.emplace_back(std::make_unique<SteamRunnerSource>());
    sources.emplace_back(std::make_unique<ManagedRunnerSource>());
    return RunnerManager(std::move(sources));
}

} // namespace rocklaunch
