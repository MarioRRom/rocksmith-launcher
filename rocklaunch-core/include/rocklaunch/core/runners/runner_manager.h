#pragma once

#include "rocklaunch/core/runners/runner_cache.h"
#include "rocklaunch/core/runners/runner_installer.h"
#include "rocklaunch/core/runners/runner_source.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

namespace rocklaunch
{

class RunnerManager
{
public:
    explicit RunnerManager(std::vector<std::unique_ptr<IRunnerSource>> sources);

    // Local discovery of installed runners.
    std::vector<Runner> List() const;
    std::optional<Runner> Find(const std::string &runnerId) const;

    // Remote cache (apt update/search layer).
    void RefreshCache(const fs::path &dataDir);
    std::vector<RunnerRelease> Search(const std::string &query,
                                      const fs::path &dataDir,
                                      bool forceRefresh = false);

    // Install/remove managed runners (apt install/remove layer).
    void Install(const std::string &runnerName,
                 const std::string &assetName,
                 const fs::path &runnersDir);
    void Remove(const std::string &runnerName,
                const fs::path &runnersDir);
    bool IsInstalled(const std::string &runnerName,
                     const fs::path &runnersDir) const;

    // Best tarball asset for the host arch for a release (used by search output).
    std::optional<AssetInfo> SelectAsset(const RunnerRelease &release) const;

    // Case-insensitive name resolution: returns the canonical tag from the cache,
    // or nullopt if no release matches (used by install/remove to normalize user input).
    std::optional<std::string> ResolveName(const std::string &runnerName,
                                           const fs::path &dataDir) const;

    static RunnerManager CreateDefault();

private:
    std::vector<std::unique_ptr<IRunnerSource>> m_sources;
    RunnerCache m_cache;
    RunnerInstaller m_installer;
};

} // namespace rocklaunch