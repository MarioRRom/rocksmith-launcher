#include "rocklaunch/core/utils/downloader.h"

#include "rocklaunch/core/logger.h"
#include "rocklaunch/core/subprocess.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace rocklaunch
{
namespace Downloader
{

namespace
{

std::string ReadFile(const fs::path &path)
{
    std::ifstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("Cannot open file: " + path.string());
    }
    return std::string((std::istreambuf_iterator<char>(f)),
                       std::istreambuf_iterator<char>());
}

// Fetch raw text from a URL using curl, writing to a temp file.
std::string CurlGet(const std::string &url)
{
    Logger logger;
    fs::path tmpDir = fs::temp_directory_path() / "rocksmith-launcher";
    fs::create_directories(tmpDir);

    fs::path tmpFile = tmpDir / "curl_response.json";

    std::error_code ec;
    fs::remove(tmpFile, ec);

    try {
        RunSubprocess({"curl", "-s", "-L", "-f",
                       "-o", tmpFile.string(),
                       "-H", "Accept: application/vnd.github+json",
                       url});
    } catch (const std::runtime_error &error) {
        // curl exits 22 on any HTTP error; api.github.com 4xx here is almost
        // always the unauthenticated 60 requests/hour rate limit.
        std::string what = error.what();
        if (what.find("exit 22") != std::string::npos
            && url.find("api.github.com") != std::string::npos) {
            throw std::runtime_error(
                "GitHub API request failed (exit 22). "
                "The unauthenticated rate limit is 60 requests/hour. "
                "Wait and retry, or use the cached releases list.");
        }
        throw;
    }

    std::string body = ReadFile(tmpFile);
    fs::remove(tmpFile, ec);
    return body;
}

ReleaseInfo ParseRelease(const nlohmann::json &rel)
{
    ReleaseInfo info;
    info.version = rel.value("tag_name", "");
    info.tag = rel.value("tag_name", "");

    if (rel.contains("assets") && rel["assets"].is_array()) {
        for (const auto &a : rel["assets"]) {
            AssetInfo asset;
            asset.name = a.value("name", "");
            asset.downloadUrl = a.value("browser_download_url", "");
            asset.size = a.value("size", 0);
            info.assets.push_back(std::move(asset));
        }
    }

    return info;
}

} // anonymous namespace

void Fetch(const std::string &url, const fs::path &destPath)
{
    Logger logger;
    logger.Info("Downloader: fetching " + url);

    fs::create_directories(destPath.parent_path());

    RunSubprocess({"curl", "-f", "-L", "--progress-bar",
                   "-o", destPath.string(),
                   url});

    if (!fs::exists(destPath) || fs::file_size(destPath) == 0) {
        throw std::runtime_error("Download failed: empty file at "
                                 + destPath.string());
    }
}

std::vector<ReleaseInfo> ListReleases(const std::string &repo, int count)
{
    Logger logger;
    logger.Info("Downloader: listing releases for " + repo);

    // GitHub's unauthenticated API is limited to 60 requests/hour per IP.
    // Every `runner search -u` calls this once per repo, so a cached release
    // list is the difference between 2 requests and hitting the limit.
    std::string url = "https://api.github.com/repos/" + repo
                    + "/releases?per_page=" + std::to_string(count);

    std::string body = CurlGet(url);

    auto json = nlohmann::json::parse(body);
    if (!json.is_array()) {
        throw std::runtime_error("GitHub API returned non-array for "
                                 + repo);
    }

    std::vector<ReleaseInfo> releases;
    releases.reserve(json.size());

    for (const auto &rel : json) {
        releases.push_back(ParseRelease(rel));
    }

    logger.Info("Downloader: found " + std::to_string(releases.size())
                + " releases for " + repo);

    return releases;
}

AssetInfo ResolveAsset(const std::string &repo, const std::string &tag,
                       const std::string &assetName)
{
    Logger logger;
    logger.Info("Downloader: resolving asset '" + assetName + "' in "
                + repo + "@" + tag);

    std::string url = "https://api.github.com/repos/" + repo
                    + "/releases/tags/" + tag;

    std::string body = CurlGet(url);

    auto json = nlohmann::json::parse(body);

    if (json.contains("assets") && json["assets"].is_array()) {
        for (const auto &a : json["assets"]) {
            if (a.value("name", "") == assetName) {
                AssetInfo asset;
                asset.name = assetName;
                asset.downloadUrl = a.value("browser_download_url", "");
                asset.size = a.value("size", 0);
                return asset;
            }
        }
    }

    throw std::runtime_error("Asset '" + assetName + "' not found in "
                             + repo + "@" + tag);
}

} // namespace Downloader
} // namespace rocklaunch
