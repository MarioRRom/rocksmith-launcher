#include "rocklaunch/core/patches/cdlc_patch.h"

#include "rocklaunch/core/config_store.h"
#include "rocklaunch/core/logger.h"
#include "rocklaunch/core/utils/downloader.h"

#include <fstream>
#include <stdexcept>

namespace rocklaunch
{

namespace
{

constexpr const char *kEnablerRepo = "Lovrom8/RSCDLCEnabler-TooManyCoresFix";
constexpr const char *kEnablerPath = "RSCDLCEnabler/D3DX9_42.dll";
constexpr const char *kEnablerFile = "D3DX9_42.dll";
constexpr const char *kCacheSubdir = "patches/cdlc-enabler";

fs::path CacheDir()
{
    return ConfigStore::DefaultDataDir() / kCacheSubdir;
}

fs::path CachedDll()
{
    return CacheDir() / kEnablerFile;
}

// Atomic copy: write to .tmp in the same directory, then rename.
void AtomicCopy(const fs::path &src, const fs::path &dst)
{
    fs::path tmpPath = dst.parent_path() / (dst.filename().string() + ".tmp");

    std::ifstream in(src, std::ios::binary);
    if (!in.is_open()) {
        throw std::runtime_error("Cannot open source file: " + src.string());
    }

    std::ofstream out(tmpPath, std::ios::binary);
    if (!out.is_open()) {
        throw std::runtime_error("Cannot write temp file: " + tmpPath.string());
    }

    out << in.rdbuf();
    out.close();
    in.close();

    std::error_code ec;
    fs::remove(dst, ec); // ensure overwrite even on platforms where rename doesn't
    fs::rename(tmpPath, dst, ec);
    if (ec) {
        throw std::runtime_error("Failed to rename " + tmpPath.string()
                                 + " to " + dst.string() + ": " + ec.message());
    }
}

void DownloadEnabler()
{
    Logger logger;
    logger.Info("CDLCPatch: downloading enabler from " + std::string(kEnablerRepo));

    fs::create_directories(CacheDir());

    // Download to a temp file first, then rename — avoids corrupt cache on
    // interrupted downloads.
    fs::path tmpPath = CachedDll();
    tmpPath += ".tmp";

    // The DLL lives in the repo tree, not in a GitHub release — fetch the raw
    // file directly from GitHub's raw content endpoint.
    std::string rawUrl = "https://raw.githubusercontent.com/" + std::string(kEnablerRepo)
                         + "/master/" + kEnablerPath;
    Downloader::Fetch(rawUrl, tmpPath);

    std::error_code ec;
    fs::rename(tmpPath, CachedDll(), ec);
    if (ec) {
        throw std::runtime_error("Failed to finalize cached enabler: "
                                 + ec.message());
    }

    logger.Info("CDLCPatch: enabler cached at " + CachedDll().string());
}

} // namespace

std::string CDLCPatch::Id() const
{
    return "cdlc-enabler";
}

std::string CDLCPatch::GameId() const
{
    return "rocksmith2014remastered";
}

PatchPreset CDLCPatch::Preset() const
{
    return {
        Id(),
        GameId(),
        "Custom Songs (CDLC)",
        "Lets custom song charts (CDLC) load by deploying the CDLC enabler DLL "
        "into the game folder, removing it when disabled.",
        true,
        true,
        {
            { PatchOperationType::CopyFile, fs::path("D3DX9_42.dll"),
              "deploy the CDLC enabler DLL next to the game executable" },
            { PatchOperationType::RemoveFile, fs::path("D3DX9_42.dll"),
              "remove the enabler DLL when disabled" },
        },
    };
}

bool CDLCPatch::IsEnabled(const ProfileConfig &profile) const
{
    const auto it = profile.patches.find(Id());
    return it != profile.patches.end() && it->second.enabled;
}

void CDLCPatch::Apply(const ProfileConfig &profile, bool force) const
{
    if (profile.gameId != GameId()) {
        throw std::runtime_error("Patch " + Id() + " is for game " + GameId()
                                 + ", profile is for game " + profile.gameId);
    }

    Logger logger;
    fs::path dllDst = profile.installDir / kEnablerFile;

    // Already deployed — nothing to do (unless forcing a re-deploy).
    if (fs::exists(dllDst) && !force) {
        logger.Info("CDLCPatch: enabler already present at " + dllDst.string());
        return;
    }

    // Ensure we have a cached copy.
    if (!fs::exists(CachedDll()) || force) {
        DownloadEnabler();
    }

    // Deploy from cache to game directory.
    AtomicCopy(CachedDll(), dllDst);
    logger.Info("CDLCPatch: enabler deployed to " + dllDst.string());
}

void CDLCPatch::Remove(const ProfileConfig &profile) const
{
    if (profile.gameId != GameId()) {
        throw std::runtime_error("Patch " + Id() + " is for game " + GameId()
                                 + ", profile is for game " + profile.gameId);
    }

    Logger logger;
    fs::path dllPath = profile.installDir / kEnablerFile;

    if (!fs::exists(dllPath)) {
        logger.Info("CDLCPatch: enabler not present, nothing to remove");
        return;
    }

    std::error_code ec;
    fs::remove(dllPath, ec);
    if (ec) {
        throw std::runtime_error("Failed to remove enabler: " + ec.message());
    }

    logger.Info("CDLCPatch: enabler removed from " + dllPath.string());
}

} // namespace rocklaunch
