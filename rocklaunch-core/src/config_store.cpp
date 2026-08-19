#include "rocklaunch/core/config_store.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <utility>

#include <nlohmann/json.hpp>

namespace rocklaunch
{

namespace
{

fs::path EnvironmentPath(const char *name)
{
    const char *value = std::getenv(name);
    return value != nullptr ? fs::path(value) : fs::path();
}

// True when path lives inside root (or is root itself). Used to keep deletions
// inside the launcher-managed directories instead of arbitrary user paths.
bool IsUnder(const fs::path &path, const fs::path &root)
{
    const fs::path relative = path.lexically_relative(root);
    return !relative.empty() && relative.native().rfind("..", 0) != 0;
}

} // namespace

ConfigStore::ConfigStore(fs::path configDir, fs::path dataDir)
    : m_configDir(std::move(configDir))
    , m_dataDir(std::move(dataDir))
{
}

fs::path ConfigStore::ConfigDir() const
{
    return m_configDir;
}

fs::path ConfigStore::DataDir() const
{
    return m_dataDir;
}

fs::path ConfigStore::ProfilePath(const std::string &profileId) const
{
    ValidateProfileId(profileId);
    return m_dataDir / "profiles" / (profileId + ".json");
}

bool ConfigStore::ProfileExists(const std::string &profileId) const
{
    std::error_code error;
    return fs::is_regular_file(ProfilePath(profileId), error);
}

std::vector<std::string> ConfigStore::ListProfileIds() const
{
    std::vector<std::string> profileIds;
    fs::path profilesDir = m_dataDir / "profiles";
    std::error_code error;
    if (!fs::is_directory(profilesDir, error)) {
        return profileIds;
    }

    for (const fs::directory_entry &entry : fs::directory_iterator(profilesDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            profileIds.emplace_back(entry.path().stem().string());
        }
    }

    std::sort(profileIds.begin(), profileIds.end());
    return profileIds;
}

std::optional<std::string> ConfigStore::ProfileUsingInstallDir(
    const fs::path &installDir, const std::string &excludedProfileId) const
{
    for (const std::string &profileId : ListProfileIds()) {
        if (profileId == excludedProfileId) {
            continue;
        }

        ProfileConfig profile = LoadProfile(profileId);
        if (profile.installDir == installDir) {
            return profileId;
        }
    }

    return std::nullopt;
}

LauncherConfig ConfigStore::LoadLauncher() const
{
    fs::path configPath = m_configDir / "config.json";
    std::ifstream input(configPath);
    if (!input.is_open()) {
        return {};
    }

    nlohmann::json json;
    input >> json;

    LauncherConfig config;
    config.settings = json.value("settings", std::map<std::string, std::string>());
    return config;
}

void ConfigStore::SaveLauncher(const LauncherConfig &config) const
{
    fs::create_directories(m_configDir);

    nlohmann::json json = {
        { "settings", config.settings },
    };
    std::ofstream output(m_configDir / "config.json");
    if (!output.is_open()) {
        throw std::runtime_error("Unable to write launcher configuration");
    }

    output << json.dump(4) << '\n';
}

ProfileConfig ConfigStore::LoadProfile(const std::string &profileId) const
{
    std::ifstream input(ProfilePath(profileId));
    if (!input.is_open()) {
        throw std::runtime_error("Unable to open profile: " + profileId);
    }

    nlohmann::json json;
    input >> json;

    ProfileConfig profile;
    profile.id = json.at("id").get<std::string>();
    profile.gameId = json.value("game_id", "rocksmith2014remastered");
    profile.installDir = json.value("install_dir", "");
    profile.runnerId = json.value("runner_id", "");
    profile.prefixDir = json.value("prefix_dir", "");
    if (json.contains("patches") && json["patches"].is_object()) {
        for (const auto &entry : json["patches"].items()) {
            PatchState state;
            state.enabled = entry.value().value("enabled", false);
            if (entry.value().contains("settings") && entry.value()["settings"].is_object()) {
                state.settings =
                    entry.value()["settings"].get<std::map<std::string, std::string>>();
            }
            profile.patches[entry.key()] = state;
        }
    }

    return profile;
}

void ConfigStore::SaveProfile(const ProfileConfig &profile) const
{
    if (profile.id.empty()) {
        throw std::invalid_argument("A profile id is required");
    }

    fs::create_directories(ProfilePath(profile.id).parent_path());

    nlohmann::json patchesJson = nlohmann::json::object();
    for (const auto &entry : profile.patches) {
        nlohmann::json patchJson = { { "enabled", entry.second.enabled } };
        if (!entry.second.settings.empty()) {
            patchJson["settings"] = entry.second.settings;
        }
        patchesJson[entry.first] = patchJson;
    }

    nlohmann::json json = {
        { "id", profile.id },
        { "game_id", profile.gameId },
        { "install_dir", profile.installDir.string() },
        { "runner_id", profile.runnerId },
        { "prefix_dir", profile.prefixDir.string() },
        { "patches", patchesJson },
    };

    std::ofstream output(ProfilePath(profile.id));
    if (!output.is_open()) {
        throw std::runtime_error("Unable to write profile: " + profile.id);
    }

    output << json.dump(4) << '\n';
}

bool ConfigStore::DeleteProfile(const std::string &profileId) const
{
    fs::path profilePath = ProfilePath(profileId);
    std::error_code error;
    bool exists = fs::is_regular_file(profilePath, error);
    if (error) {
        throw std::runtime_error("Unable to delete profile: " + profileId);
    }

    std::optional<ProfileConfig> profile;
    if (exists) {
        profile = LoadProfile(profileId);
    }

    bool removed = fs::remove(profilePath, error);
    if (error) {
        throw std::runtime_error("Unable to delete profile: " + profileId);
    }

    if (removed && profile.has_value()) {
        RemovePrefixDir(profile->prefixDir);
    }

    return removed;
}

void ConfigStore::RemovePrefixDir(const fs::path &prefixDir) const
{
    fs::path prefixesDir = DefaultDataDir() / "prefixes";
    std::error_code error;
    if (!IsUnder(prefixDir, prefixesDir) || !fs::is_directory(prefixDir, error)) {
        return;
    }

    fs::remove_all(prefixDir, error);
}

void ConfigStore::ValidateProfileId(const std::string &profileId) const
{
    if (profileId.empty()) {
        throw std::invalid_argument("A profile id is required");
    }

    for (char character : profileId) {
        bool isLetter = character >= 'a' && character <= 'z';
        bool isDigit = character >= '0' && character <= '9';
        if (!isLetter && !isDigit && character != '-' && character != '_') {
            throw std::invalid_argument(
                "Profile ids may contain only lowercase letters, numbers, hyphens, and "
                "underscores");
        }
    }
}

fs::path ConfigStore::DefaultConfigDir()
{
    fs::path configHome = EnvironmentPath("XDG_CONFIG_HOME");
    if (!configHome.empty()) {
        return configHome / "rocksmith-launcher";
    }

    fs::path homeDir = EnvironmentPath("HOME");
    if (!homeDir.empty()) {
        return homeDir / ".config" / "rocksmith-launcher";
    }

    throw std::runtime_error("Neither XDG_CONFIG_HOME nor HOME is set");
}

fs::path ConfigStore::DefaultDataDir()
{
    fs::path dataHome = EnvironmentPath("XDG_DATA_HOME");
    if (!dataHome.empty()) {
        return dataHome / "rocksmith-launcher";
    }

    fs::path homeDir = EnvironmentPath("HOME");
    if (!homeDir.empty()) {
        return homeDir / ".local" / "share" / "rocksmith-launcher";
    }

    throw std::runtime_error("Neither XDG_DATA_HOME nor HOME is set");
}

} // namespace rocklaunch
