#include "rocklaunch/core/runners/runner_manager.h"

#include "rocklaunch/core/runners/launcher_runner_source.h"
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
    for (const Runner &runner : List()) {
        if (runner.id == runnerId) {
            return runner;
        }
    }

    return std::nullopt;
}

void RunnerManager::RefreshCache(const fs::path &dataDir)
{
    m_cache.Refresh(dataDir);
}

std::vector<RunnerRelease> RunnerManager::Search(const std::string &query,
                                                 const fs::path &dataDir,
                                                 bool forceRefresh)
{
    return m_cache.Search(query, dataDir, forceRefresh);
}

void RunnerManager::Install(const std::string &runnerName,
                            const std::string &assetName,
                            const fs::path &runnersDir)
{
    m_installer.Install(runnerName, assetName, runnersDir, m_cache);
}

void RunnerManager::Remove(const std::string &runnerName,
                           const fs::path &runnersDir)
{
    m_installer.Remove(runnerName, runnersDir);
}

bool RunnerManager::IsInstalled(const std::string &runnerName,
                                const fs::path &runnersDir) const
{
    // Resolve the canonical name from the cache so case-insensitive lookups
    // match the directory that Install() creates.
    fs::path dataDir = runnersDir.parent_path();
    std::optional<std::string> resolved = ResolveName(runnerName, dataDir);
    std::string canonical = resolved.value_or(runnerName);
    return m_installer.IsInstalled(canonical, runnersDir);
}

std::optional<AssetInfo> RunnerManager::SelectAsset(const RunnerRelease &release) const
{
    return m_installer.SelectAsset(release);
}

std::optional<std::string> RunnerManager::ResolveName(const std::string &runnerName,
                                                      const fs::path &dataDir) const
{
    try {
        std::vector<RunnerRelease> releases = m_cache.ReadAll(dataDir);
        const RunnerRelease *release = m_installer.ResolveRelease(releases, runnerName);
        if (release != nullptr) {
            return release->tag;
        }
    } catch (...) {
        // Cache not available — fall back to the raw name.
    }
    return std::nullopt;
}

RunnerManager RunnerManager::CreateDefault()
{
    std::vector<std::unique_ptr<IRunnerSource>> sources;
    sources.emplace_back(std::make_unique<SystemWineSource>());
    sources.emplace_back(std::make_unique<SteamRunnerSource>());
    sources.emplace_back(std::make_unique<LauncherRunnerSource>());
    return RunnerManager(std::move(sources));
}

} // namespace rocklaunch