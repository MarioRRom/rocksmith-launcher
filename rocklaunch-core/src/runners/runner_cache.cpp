#include "rocklaunch/core/runners/runner_cache.h"

#include "rocklaunch/core/logger.h"
#include "rocklaunch/core/utils/downloader.h"
#include "rocklaunch/core/utils/string_util.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <istream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace rocklaunch
{

namespace
{

const int kCacheTtlHours = 24;

// All configured runner repos (apt sources.list equivalent).
std::vector<std::string> GetRunnerRepos()
{
    return {
        "GloriousEggroll/proton-ge-custom",
        "CachyOS/Proton-CachyOS",
    };
}

std::vector<std::string> SplitQuery(const std::string &query)
{
    std::istringstream ss(query);
    std::vector<std::string> terms;
    std::string term;
    while (ss >> term) {
        terms.push_back(term);
    }
    return terms;
}

std::string ReadFileContent(const fs::path &path)
{
    std::ifstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("Cannot open file: " + path.string());
    }
    return std::string((std::istreambuf_iterator<char>(f)),
                       std::istreambuf_iterator<char>());
}

// Single source of truth for the runner_releases.json schema.
nlohmann::json ReleaseToJson(const RunnerRelease &rr)
{
    nlohmann::json entry;
    entry["repo"] = rr.repo;
    entry["tag"] = rr.tag;
    entry["assets"] = nlohmann::json::array();
    for (const AssetInfo &asset : rr.assets) {
        entry["assets"].push_back({
            { "name", asset.name },
            { "download_url", asset.downloadUrl },
            { "size", asset.size },
        });
    }
    return entry;
}

RunnerRelease ReleaseFromJson(const nlohmann::json &entry)
{
    RunnerRelease rr;
    rr.repo = entry.value("repo", "");
    rr.tag = entry.value("tag", "");

    if (entry.contains("assets") && entry["assets"].is_array()) {
        for (const auto &a : entry["assets"]) {
            AssetInfo asset;
            asset.name = a.value("name", "");
            asset.downloadUrl = a.value("download_url", "");
            asset.size = a.value("size", 0);
            rr.assets.push_back(std::move(asset));
        }
    }

    return rr;
}

} // anonymous namespace

fs::path RunnerCache::CachePath(const fs::path &dataDir)
{
    return dataDir / "runner_releases.json";
}

std::string RunnerCache::CurrentTimestamp() const
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&time));
    return buf;
}

void RunnerCache::Refresh(const fs::path &dataDir)
{
    Logger logger;
    logger.Info("RunnerCache: refreshing release cache");

    std::vector<RunnerRelease> all;
    for (const std::string &repo : GetRunnerRepos()) {
        std::vector<ReleaseInfo> releases =
            Downloader::ListReleases(repo, 100);
        for (const ReleaseInfo &rel : releases) {
            RunnerRelease rr;
            rr.repo = repo;
            rr.tag = rel.tag;
            rr.assets = rel.assets;
            all.push_back(std::move(rr));
        }
    }

    nlohmann::json json;
    json["fetched_at"] = CurrentTimestamp();
    json["releases"] = nlohmann::json::array();
    for (const RunnerRelease &rr : all) {
        json["releases"].push_back(ReleaseToJson(rr));
    }

    fs::create_directories(dataDir);
    fs::path path = CachePath(dataDir);
    std::ofstream out(path);
    if (!out.is_open()) {
        throw std::runtime_error("Cannot write cache: " + path.string());
    }
    out << json.dump(4) << '\n';

    logger.Info("RunnerCache: cached " + std::to_string(all.size()) + " releases");
}

bool RunnerCache::NeedsRefresh(const fs::path &path) const
{
    if (!fs::is_regular_file(path)) {
        return true;
    }

    std::string content = ReadFileContent(path);
    auto json = nlohmann::json::parse(content);
    std::string fetchedAt = json.value("fetched_at", "");

    std::tm tm = {};
    std::istringstream ss(fetchedAt);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");

    // fetched_at is written in UTC (gmtime), so parse it as UTC with timegm.
    auto fileTime = std::chrono::system_clock::from_time_t(timegm(&tm));
    auto now = std::chrono::system_clock::now();
    auto ageHours = std::chrono::duration_cast<std::chrono::hours>(now - fileTime).count();
    return ageHours >= kCacheTtlHours;
}

std::vector<RunnerRelease> RunnerCache::ParseJson(const std::string &content) const
{
    auto json = nlohmann::json::parse(content);

    std::vector<RunnerRelease> result;
    for (const auto &entry : json["releases"]) {
        result.push_back(ReleaseFromJson(entry));
    }

    return result;
}

std::vector<RunnerRelease> RunnerCache::ReadAll(const fs::path &dataDir) const
{
    fs::path path = CachePath(dataDir);
    if (!fs::is_regular_file(path)) {
        throw std::runtime_error(
            "No runner cache found. Run 'runner -u' first.");
    }

    return ParseJson(ReadFileContent(path));
}

std::vector<RunnerRelease> RunnerCache::Search(const std::string &query,
                                               const fs::path &dataDir,
                                               bool forceRefresh)
{
    Logger logger;
    fs::path path = CachePath(dataDir);

    if (forceRefresh || NeedsRefresh(path)) {
        Refresh(dataDir);
    }

    std::vector<RunnerRelease> all = ParseJson(ReadFileContent(path));

    std::vector<std::string> terms = SplitQuery(query);
    std::vector<RunnerRelease> result;
    for (RunnerRelease &rr : all) {
        std::string haystack = ToLower(rr.repo + " " + rr.tag);
        bool match = true;
        for (const std::string &term : terms) {
            if (haystack.find(ToLower(term)) == std::string::npos) {
                match = false;
                break;
            }
        }
        if (match) {
            result.push_back(std::move(rr));
        }
    }

    return result;
}

} // namespace rocklaunch