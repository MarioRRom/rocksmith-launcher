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

// A single game installation. One profile owns one install dir, its runtime and patches.
struct ProfileConfig
{
    std::string id;
    std::string gameId;
    fs::path installDir;
    std::string runtimeId;
    fs::path prefixDir;
    std::map<std::string, bool> patches;
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
    bool DeleteProfile(const std::string &profileId) const;

    static fs::path DefaultConfigDir();
    static fs::path DefaultDataDir();

private:
    // Keeps profile ids filesystem-safe; they are used to build file names.
    void ValidateProfileId(const std::string &profileId) const;

    fs::path m_configDir;
};

} // namespace rocklaunch
