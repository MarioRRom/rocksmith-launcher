#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace rocklaunch
{

namespace fs = std::filesystem;

// Launcher-wide settings shared by every profile.
struct LauncherConfig
{
    std::map<std::string, std::string> settings;
};

// Per-patch state persisted in a profile. Each patch owns its own section in the
// profile JSON; the manager only reads/writes the enabled flag, concrete patches
// may store extra fields later.
struct PatchState
{
    bool enabled = false;
    std::map<std::string, std::string> settings;
};

// A single game installation. One profile owns one install dir, its runner and patches.
struct ProfileConfig
{
    std::string id;
    std::string gameId;
    fs::path installDir;
    std::string runnerId;
    fs::path prefixDir;
    std::map<std::string, PatchState> patches;
};

// JSON persistence for the launcher configuration and its per-installation profiles.
class ConfigStore
{
public:
    explicit ConfigStore(fs::path configDir = DefaultConfigDir());

    fs::path ConfigDir() const;
    fs::path ProfilePath(const std::string &profileId) const;
    bool ProfileExists(const std::string &profileId) const;
    std::vector<std::string> ListProfileIds() const;
    // Profile that already claims installDir, if any. Excludes excludedProfileId from the search.
    std::optional<std::string> ProfileUsingInstallDir(
        const fs::path &installDir, const std::string &excludedProfileId = {}) const;
    LauncherConfig LoadLauncher() const;
    void SaveLauncher(const LauncherConfig &config) const;
    ProfileConfig LoadProfile(const std::string &profileId) const;
    void SaveProfile(const ProfileConfig &profile) const;
    // Deletes the profile configuration and, when its prefix lives under the
    // managed data directory, the prefix itself. Returns false when the profile
    // did not exist.
    bool DeleteProfile(const std::string &profileId) const;

    static fs::path DefaultConfigDir();
    static fs::path DefaultDataDir();

private:
    // Keeps profile ids filesystem-safe; they are used to build file names.
    void ValidateProfileId(const std::string &profileId) const;
    // Removes prefixDir only when it lives under the launcher-managed prefixes dir.
    void RemovePrefixDir(const fs::path &prefixDir) const;

    fs::path m_configDir;
};

} // namespace rocklaunch
