#include "rocklaunch/core/runners/runner_installer.h"

#include "rocklaunch/core/logger.h"
#include "rocklaunch/core/subprocess.h"
#include "rocklaunch/core/utils/checksum.h"
#include "rocklaunch/core/utils/downloader.h"
#include "rocklaunch/core/utils/string_util.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>
#include <sys/utsname.h>
#include <unistd.h>

namespace rocklaunch
{

namespace
{

bool IsTarball(const std::string &nameLower)
{
    static const std::vector<std::string> kTarballExtensions = {
        ".tar.gz", ".tar.xz", ".tar.zst", ".tar.bz2", ".tgz",
    };
    for (const std::string &ext : kTarballExtensions) {
        if (EndsWith(nameLower, ext)) {
            return true;
        }
    }
    return false;
}

// Whole-token arch match: "x86_64" must not match "x86_64_v3".
bool MatchesArch(const std::string &nameLower, const std::string &archLower)
{
    std::size_t pos = nameLower.find(archLower);
    while (pos != std::string::npos) {
        std::size_t after = pos + archLower.size();
        bool boundary = after >= nameLower.size()
            || !(std::isalnum(static_cast<unsigned char>(nameLower[after]))
                 || nameLower[after] == '_');
        if (boundary) {
            return true;
        }
        pos = nameLower.find(archLower, pos + 1);
    }
    return false;
}

// True when the name explicitly targets a different architecture.
bool IsForOtherArch(const std::string &nameLower, const std::string &hostArch)
{
    static const std::vector<std::string> kKnownArches = {
        "x86_64", "aarch64", "arm64", "amd64", "i386", "i686", "armv7l", "armv8l",
    };
    for (const std::string &arch : kKnownArches) {
        if (arch != hostArch && MatchesArch(nameLower, arch)) {
            return true;
        }
    }
    return false;
}

// Preferred format when the same build ships in several compression formats.
int TarballPriority(const std::string &nameLower)
{
    if (EndsWith(nameLower, ".tar.gz")) {
        return 0;
    }
    if (EndsWith(nameLower, ".tar.xz")) {
        return 1;
    }
    if (EndsWith(nameLower, ".tar.bz2")) {
        return 2;
    }
    if (EndsWith(nameLower, ".tar.zst")) {
        return 3;
    }
    if (EndsWith(nameLower, ".tgz")) {
        return 4;
    }
    return 100;
}

} // anonymous namespace

void RunnerInstaller::Install(const std::string &runnerName,
                              const std::string &assetName,
                              const fs::path &runnersDir,
                              const RunnerCache &cache)
{
    Logger logger;

    // Install works strictly from the cached release list (apt-like).
    // Match the exact tag; the repo comes from the release itself.
    // The cache lives next to the runners dir: <dataDir>/runner_releases.json.
    fs::path dataDir = runnersDir.parent_path();
    std::vector<RunnerRelease> releases = cache.ReadAll(dataDir);
    const RunnerRelease *release = ResolveRelease(releases, runnerName);
    if (release == nullptr) {
        throw std::runtime_error(
            "No release named '" + runnerName + "' in cache. "
            "Use 'runner search <query>' to browse available releases.");
    }

    // Use the canonical tag for paths and messages — the user may have typed
    // a different case, and the folder must always match the upstream tag.
    const std::string &tag = release->tag;

    logger.Info("RunnerInstaller: installing " + tag);

    // Resolve the asset to download.
    std::string selectedAsset = assetName;
    if (selectedAsset.empty()) {
        std::optional<AssetInfo> autoAsset = SelectAsset(*release);
        if (autoAsset.has_value()) {
            selectedAsset = autoAsset->name;
        }
        if (selectedAsset.empty()) {
            throw std::runtime_error(
                "Cannot auto-detect asset for " + tag + ". "
                "Use 'runner install <name> <asset>' to specify manually.");
        }
    }

    AssetInfo asset =
        Downloader::ResolveAsset(release->repo, release->tag, selectedAsset);

    // Find the sha512sum asset from the release.
    std::optional<AssetInfo> sha512Asset = FindSha512Asset(*release);
    if (!sha512Asset.has_value()) {
        throw std::runtime_error(
            "No sha512sum asset found for " + release->tag + ". "
            "Cannot verify download integrity.");
    }

    // Temp dir under runnersDir keeps the final rename on the same filesystem.
    fs::path tmpDir = runnersDir / (".tmp-download-" + std::to_string(getpid()));
    fs::create_directories(tmpDir);

    try {
        // Download tarball and its .sha512sum file.
        fs::path tarballPath = tmpDir / asset.name;
        Downloader::Fetch(asset.downloadUrl, tarballPath);
        fs::path sha512Path = tmpDir / sha512Asset->name;
        Downloader::Fetch(sha512Asset->downloadUrl, sha512Path);

        // Verify the tarball against the expected hash.
        std::string expectedHash = ParseSha512SumFile(sha512Path);
        std::string actualHash = HashFile(tarballPath);
        if (actualHash != expectedHash) {
            logger.Error("RunnerInstaller: SHA-512 mismatch for " + asset.name);
            throw std::runtime_error(
                "SHA-512 mismatch for " + asset.name + "\n"
                "  expected: " + expectedHash + "\n"
                "  got:      " + actualHash);
        }
        logger.Debug("RunnerInstaller: SHA-512 verified for " + asset.name);

        // Extract and move the top-level directory into place.
        fs::path extractDir = tmpDir / "extracted";
        fs::create_directories(extractDir);
        RunSubprocess({"tar", "-xf", tarballPath.string(), "-C", extractDir.string()});

        std::optional<fs::path> extractedDir;
        for (const fs::directory_entry &entry : fs::directory_iterator(extractDir)) {
            if (entry.is_directory()) {
                extractedDir = entry.path();
                break;
            }
        }
        if (!extractedDir.has_value()) {
            throw std::runtime_error("Extraction produced no directory");
        }

        fs::path finalPath = runnersDir / tag;
        fs::remove_all(finalPath);
        fs::rename(*extractedDir, finalPath);

        fs::remove_all(tmpDir);

        logger.Debug("RunnerInstaller: installed " + tag + " to "
                     + finalPath.string());
        logger.Info("RunnerInstaller: installed " + tag);
    } catch (...) {
        fs::remove_all(tmpDir);
        throw;
    }
}

void RunnerInstaller::Remove(const std::string &runnerName,
                             const fs::path &runnersDir)
{
    Logger logger;

    fs::path runnerDir = runnersDir / runnerName;
    std::error_code isDirError;
    if (!fs::is_directory(runnerDir, isDirError)) {
        throw std::runtime_error("Runner not installed: " + runnerName);
    }

    // Safety check: ensure it's under the expected runners directory.
    std::error_code canonError;
    fs::path canonical = fs::canonical(runnerDir, canonError);
    if (canonError) {
        throw std::runtime_error("Cannot resolve runner path: " + runnerName);
    }

    std::error_code canonBaseError;
    fs::path canonicalBase = fs::canonical(runnersDir, canonBaseError);
    if (canonBaseError) {
        throw std::runtime_error("Cannot resolve runners directory");
    }

    fs::path relative = canonical.lexically_relative(canonicalBase);
    if (relative.empty() || relative.native().rfind("..", 0) == 0) {
        throw std::runtime_error("Refusing to remove outside runners directory: "
                                 + runnerName);
    }

    std::error_code removeError;
    fs::remove_all(runnerDir, removeError);
    if (removeError) {
        throw std::runtime_error("Failed to remove runner: " + runnerName);
    }

    logger.Info("RunnerInstaller: removed " + runnerName);
}

bool RunnerInstaller::IsInstalled(const std::string &runnerName,
                                  const fs::path &runnersDir) const
{
    std::error_code error;
    return fs::is_directory(runnersDir / runnerName, error);
}

const RunnerRelease *RunnerInstaller::ResolveRelease(
    const std::vector<RunnerRelease> &releases,
    const std::string &runnerName)
{
    std::string lower = ToLower(runnerName);
    for (const RunnerRelease &rr : releases) {
        if (ToLower(rr.tag) == lower) {
            return &rr;
        }
    }
    return nullptr;
}

std::optional<AssetInfo> RunnerInstaller::SelectAsset(const RunnerRelease &release) const
{
    // Tarballs only; never .sha512sum or source archives.
    std::vector<AssetInfo> candidates;
    for (const AssetInfo &asset : release.assets) {
        std::string nameLower = ToLower(asset.name);
        if (IsTarball(nameLower)
            && nameLower.find("sha512") == std::string::npos) {
            candidates.push_back(asset);
        }
    }

    if (candidates.size() == 1) {
        return candidates[0];
    }

    auto filter = [](const std::vector<AssetInfo> &source, auto keep) {
        std::vector<AssetInfo> result;
        for (const AssetInfo &asset : source) {
            if (keep(ToLower(asset.name))) {
                result.push_back(asset);
            }
        }
        return result;
    };

    // Match the host architecture token exactly.
    std::string hostArch = ToLower(DetectHostArch());
    std::vector<AssetInfo> matched = filter(
        candidates, [&](const std::string &name) { return MatchesArch(name, hostArch); });
    if (matched.size() == 1) {
        return matched[0];
    }

    // No arch token matched: exclude assets for a different arch, then prefer
    // the canonical compression format.
    std::vector<AssetInfo> filtered = filter(
        candidates, [&](const std::string &name) { return !IsForOtherArch(name, hostArch); });
    if (filtered.size() == 1) {
        return filtered[0];
    }
    if (filtered.size() > 1) {
        std::stable_sort(filtered.begin(), filtered.end(),
                         [](const AssetInfo &left, const AssetInfo &right) {
                             return TarballPriority(ToLower(left.name))
                                 < TarballPriority(ToLower(right.name));
                         });
        return filtered[0];
    }

    return std::nullopt;
}

std::string RunnerInstaller::DetectHostArch()
{
    struct utsname uts;
    if (uname(&uts) == 0) {
        return uts.machine;
    }
    return "x86_64";
}

std::optional<AssetInfo> RunnerInstaller::FindSha512Asset(
    const RunnerRelease &release)
{
    std::vector<AssetInfo> candidates;
    for (const AssetInfo &asset : release.assets) {
        std::string nameLower = ToLower(asset.name);
        if (EndsWith(nameLower, ".sha512sum")) {
            candidates.push_back(asset);
        }
    }

    if (candidates.size() == 1) {
        return candidates[0];
    }

    if (candidates.size() > 1) {
        std::string hostArch = ToLower(DetectHostArch());
        std::vector<AssetInfo> matched;
        for (const AssetInfo &asset : candidates) {
            std::string nameLower = ToLower(asset.name);
            if (MatchesArch(nameLower, hostArch)) {
                matched.push_back(asset);
            }
        }
        if (matched.size() == 1) {
            return matched[0];
        }
    }

    return std::nullopt;
}

std::string RunnerInstaller::ParseSha512SumFile(const fs::path &path)
{
    std::ifstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("Cannot open file: " + path.string());
    }

    std::string line;
    if (std::getline(f, line) && !line.empty()) {
        // sha512sum format: "<hash>  <filename>\n"
        std::size_t spacePos = line.find(' ');
        if (spacePos != std::string::npos) {
            return line.substr(0, spacePos);
        }
        return line;
    }
    throw std::runtime_error("Cannot parse sha512sum file: " + path.string());
}

} // namespace rocklaunch