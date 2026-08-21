#pragma once

#include "rocklaunch/core/utils/downloader.h"

#include <filesystem>
#include <string>
#include <vector>

namespace rocklaunch
{

struct RunnerRelease
{
    std::string repo;   // GitHub "owner/repo" slug the release belongs to
    std::string tag;
    std::vector<AssetInfo> assets;
};

// Cache of releases fetched from all configured repos (apt update/search layer).
// Persists the combined result to <dataDir>/runner_releases.json.
class RunnerCache
{
public:
    // Fetch all registry repos and save the combined result to disk.
    void Refresh(const fs::path &dataDir);

    // Read cached releases. Refreshes automatically if the cache is stale (>24h)
    // or if forceRefresh is true. Filters by case-insensitive substring on
    // "repo tag"; every whitespace-separated term must match (AND).
    std::vector<RunnerRelease> Search(const std::string &query,
                                      const fs::path &dataDir,
                                      bool forceRefresh = false);

    // Read cached releases without refreshing. Throws when the cache is missing
    // (used by install, which works strictly from the cache like apt install).
    std::vector<RunnerRelease> ReadAll(const fs::path &dataDir) const;

private:
    static fs::path CachePath(const fs::path &dataDir);

    bool NeedsRefresh(const fs::path &path) const;
    std::vector<RunnerRelease> ParseJson(const std::string &content) const;
    std::string CurrentTimestamp() const;
};

} // namespace rocklaunch