#include "rocklaunch/core/config_store.h"
#include "rocklaunch/core/logger.h"
#include "rocklaunch/core/manual_source.h"
#include "rocklaunch/core/rocksmith2014_profile.h"

#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace
{

void PrintUsage()
{
    std::cout << "Usage: rocklaunch-cli <command> [arguments]\n"
              << "\n"
              << "Commands:\n"
              << "  profile list                       List configured profiles.\n"
              << "  profile new [<profile>]            Create a profile (auto-named <game>-<n> if omitted).\n"
              << "  profile show <profile>             Show a profile's settings.\n"
              << "  profile delete <profile>           Delete a profile configuration.\n"
              << "  set-path <profile> <path>          Validate and save a game installation.\n"
              << "  --help, -h          Show this help message.\n"
              << "  --version           Show the launcher version.\n";
}

// First unused "<gameId>-<n>" name so bare `profile new` keeps creating distinct profiles.
std::string NextDefaultProfileId(const std::string &gameId,
                                 const rocklaunch::ConfigStore &configStore)
{
    for (int index = 1;; ++index) {
        std::string candidate = gameId + "-" + std::to_string(index);
        if (!configStore.ProfileExists(candidate)) {
            return candidate;
        }
    }
}

// Profile commands

int CreateProfile(const std::string &profileId,
                  rocklaunch::ConfigStore &configStore,
                  const rocklaunch::Rocksmith2014Profile &gameProfile)
{
    if (!configStore.ProfileExists(profileId)) {
        rocklaunch::ProfileConfig config;
        config.id = profileId;
        config.gameId = gameProfile.Id();
        configStore.SaveProfile(config);
        std::cout << "Created profile: " << profileId << '\n';
        return 0;
    }

    std::cerr << "Profile already exists: " << profileId << '\n';
    return 1;
}

int ShowProfile(const std::string &profileId, rocklaunch::ConfigStore &configStore)
{
    if (!configStore.ProfileExists(profileId)) {
        std::cerr << "Profile not found: " << profileId << '\n';
        return 1;
    }

    rocklaunch::ProfileConfig config = configStore.LoadProfile(profileId);
    std::cout << "Profile: " << config.id << '\n'
              << "Game: " << config.gameId << '\n'
              << "Install path: ";
    if (config.installDir.empty()) {
        std::cout << "not assigned\n";
    } else {
        std::cout << config.installDir << '\n';
    }

    return 0;
}

int DeleteProfile(const std::string &profileId, rocklaunch::ConfigStore &configStore)
{
    if (!configStore.DeleteProfile(profileId)) {
        std::cerr << "Profile not found: " << profileId << '\n';
        return 1;
    }

    std::cout << "Deleted profile configuration: " << profileId << '\n';
    return 0;
}

// Path assignment

int SetPath(const std::string &profileId,
            const char *path,
            rocklaunch::ConfigStore &configStore,
            const rocklaunch::Rocksmith2014Profile &gameProfile)
{
    if (!configStore.ProfileExists(profileId)) {
        std::cerr << "Profile not found: " << profileId << '\n'
                  << "Create it first with profile new <profile>.\n";
        return 1;
    }

    rocklaunch::ManualSource source(path);
    std::optional<rocklaunch::fs::path> installDir = source.Locate(gameProfile);
    if (!installDir.has_value()) {
        std::cerr << "Invalid Rocksmith 2014 installation: " << path << '\n'
                  << "Expected Rocksmith2014.exe and a dlc directory.\n";
        return 1;
    }

    std::optional<std::string> conflictingProfile =
        configStore.ProfileUsingInstallDir(*installDir, profileId);
    if (conflictingProfile.has_value()) {
        std::cerr << "This game installation is already used by profile: "
                  << *conflictingProfile << '\n';
        return 1;
    }

    rocklaunch::ProfileConfig config = configStore.LoadProfile(profileId);

    config.installDir = *installDir;
    configStore.SaveProfile(config);

    std::cout << "Saved profile " << profileId << ": " << *installDir << '\n';
    return 0;
}

int ListProfiles(rocklaunch::ConfigStore &configStore)
{
    std::vector<std::string> profileIds = configStore.ListProfileIds();
    if (profileIds.empty()) {
        std::cout << "No profiles are configured.\n";
        return 0;
    }

    for (const std::string &profileId : profileIds) {
        std::cout << profileId << '\n';
    }

    return 0;
}

} // namespace

// Entry point

int main(int argc, char *argv[])
{
    try {
        rocklaunch::Logger logger;
        logger.Info("rocklaunch-cli started");
        rocklaunch::ConfigStore configStore;
        rocklaunch::Rocksmith2014Profile profile;

        if (argc == 1) {
            PrintUsage();
            return 0;
        }

        std::string_view argument(argv[1]);
        if (argument == "--help" || argument == "-h") {
            PrintUsage();
            return 0;
        }

        if (argument == "--version") {
            std::cout << "rocklaunch-cli " << ROCKLAUNCH_VERSION << '\n';
            return 0;
        }

        if (argument == "set-path" && argc == 4) {
            logger.Info("Saving a manual game installation");
            return SetPath(argv[2], argv[3], configStore, profile);
        }

        if (argument == "profile" && argc == 3 && std::string_view(argv[2]) == "list") {
            logger.Info("Listing configured profiles");
            return ListProfiles(configStore);
        }

        if (argument == "profile" && argc == 3 && std::string_view(argv[2]) == "new") {
            logger.Info("Creating a profile with an auto-generated name");
            std::string profileId = NextDefaultProfileId(profile.Id(), configStore);
            return CreateProfile(profileId, configStore, profile);
        }

        if (argument == "profile" && argc == 4 && std::string_view(argv[2]) == "new") {
            logger.Info("Creating a profile");
            return CreateProfile(argv[3], configStore, profile);
        }

        if (argument == "profile" && argc == 4 && std::string_view(argv[2]) == "show") {
            logger.Info("Showing a profile");
            return ShowProfile(argv[3], configStore);
        }

        if (argument == "profile" && argc == 4 && std::string_view(argv[2]) == "delete") {
            logger.Info("Deleting a profile configuration");
            return DeleteProfile(argv[3], configStore);
        }

        std::cerr << "Unknown option: " << argument << '\n';
        PrintUsage();
        return 1;
    } catch (const std::exception &error) {
        std::cerr << "rocklaunch-cli: " << error.what() << '\n';
        return 1;
    }
}
