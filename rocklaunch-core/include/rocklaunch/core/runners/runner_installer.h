#pragma once

#include "rocklaunch/core/runners/runner_cache.h"
#include "rocklaunch/core/utils/downloader.h"

#include <filesystem>
#include <optional>
#include <string>

namespace rocklaunch
{

// Download, verify, extract, and remove managed runners (apt install/remove layer).
// Install works strictly from the cache, like `apt install` works from the
// local package list.
class RunnerInstaller
{
public:
    // Download, SHA-512 verify, extract, and install a runner.
    // assetName is optional; auto-detects by host arch if omitted.
    void Install(const std::string &runnerName,
                 const std::string &assetName,
                 const fs::path &runnersDir,
                 const RunnerCache &cache);

    // Delete a managed runner directory.
    void Remove(const std::string &runnerName,
                const fs::path &runnersDir);

    // True if a managed runner with this name is installed.
    bool IsInstalled(const std::string &runnerName,
                     const fs::path &runnersDir) const;

    // Select the best tarball asset for the host architecture.
    // Returns nullopt when ambiguous (caller should ask for --asset).
    std::optional<AssetInfo> SelectAsset(const RunnerRelease &release) const;

    // Find the release in a list matching a runner name (case-insensitive tag).
    static const RunnerRelease *ResolveRelease(
        const std::vector<RunnerRelease> &releases,
        const std::string &runnerName);

private:
    // Find the .sha512sum asset for a release, disambiguating by host arch.
    static std::optional<AssetInfo> FindSha512Asset(const RunnerRelease &release);

    // Detect host architecture via uname() (e.g. "x86_64").
    static std::string DetectHostArch();

    // Parse a sha512sum file and return the hex hash (first token of first line).
    static std::string ParseSha512SumFile(const fs::path &path);
};

} // namespace rocklaunch