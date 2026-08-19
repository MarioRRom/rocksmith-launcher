#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace rocklaunch
{

namespace fs = std::filesystem;

struct AssetInfo
{
    std::string name;
    std::string downloadUrl;
    uint64_t size;
};

struct ReleaseInfo
{
    std::string version;
    std::string tag;
    std::string publishedAt;
    std::vector<AssetInfo> assets;
};

// HTTP download and GitHub release utilities. All functions use curl
// via subprocess — no library dependency beyond curl on PATH.
namespace Downloader
{

// Download a file from url to destPath. Shows curl progress bar.
// Throws on HTTP error or network failure.
void Fetch(const std::string &url, const fs::path &destPath);

// List releases from a GitHub repo (e.g. "GloriousEggroll/proton-ge-custom").
// Returns up to count releases, newest first.
std::vector<ReleaseInfo> ListReleases(const std::string &repo,
                                      int count = 30);

// Find an asset by name (exact match) in a specific release tag.
// Returns the AssetInfo with the download URL, or throws if not found.
AssetInfo ResolveAsset(const std::string &repo, const std::string &tag,
                       const std::string &assetName);

} // namespace Downloader
} // namespace rocklaunch
