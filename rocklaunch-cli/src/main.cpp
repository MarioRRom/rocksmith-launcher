#include "rocklaunch/core/config_store.h"
#include "rocklaunch/core/launch.h"
#include "rocklaunch/core/logger.h"
#include "rocklaunch/core/manual_source.h"
#include "rocklaunch/core/patches/patch_manager.h"
#include "rocklaunch/core/rocksmith2014_remastered_profile.h"
#include "rocklaunch/core/runners/runner_manager.h"

#include <cstring>
#include <exception>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace
{

void PrintUsageEntry(const std::string &command, const std::string &description)
{
    std::cout << "    " << std::left << std::setw(40) << command << description << '\n';
}

void PrintUsage()
{
    std::cout << "rocklaunch-cli — Linux launcher for Rocksmith 2014\n"
              << "\n"
              << "USAGE:\n"
              << "    rocklaunch-cli <command> [options]\n"
              << "\n"
              << "PROFILES:\n";
    PrintUsageEntry("profile list", "List all profiles.");
    PrintUsageEntry("profile new [<name>]", "Create a profile (auto-named if omitted).");
    PrintUsageEntry("profile show <profile>", "Show profile details.");
    PrintUsageEntry("profile remove <profile>", "Remove a profile and its prefix.");
    PrintUsageEntry("set-path <profile> <path>", "Validate and set the game install path.");
    std::cout << "\nRUNNERS:\n";
    PrintUsageEntry("runner list", "List available Wine and Proton runners.");
    PrintUsageEntry("runner set <profile> <runner_id>", "Assign a runner to a profile.");
    std::cout << "\nPATCHES:\n";
    PrintUsageEntry("patch list", "List all available patches.");
    PrintUsageEntry("patch list <profile>", "List patches for a profile.");
    PrintUsageEntry("patch add [-f] <profile> <patch>", "Enable a patch (-f to force re-enable).");
    PrintUsageEntry("patch remove [-f] <profile> <patch>", "Disable a patch (-f to force).");
    PrintUsageEntry("patch status <profile> <patch>", "Show a patch's state.");
    std::cout << "\nLAUNCH:\n";
    PrintUsageEntry("launch <profile>", "Launch the profile's game in its prefix.");
    std::cout << "\nGENERAL:\n";
    PrintUsageEntry("--help, -h", "Show this help message.");
    PrintUsageEntry("--version", "Show the launcher version.");
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

// Confirmation for destructive commands. CLI-only; the core never prompts.
bool ConfirmDestructive(const std::string &what)
{
    std::cout << what << " [y/N] ";
    std::cout.flush();

    std::string answer;
    std::getline(std::cin, answer);
    return answer == "y" || answer == "Y" || answer == "yes" || answer == "Yes"
        || answer == "YES";
}

// Profile commands

int CreateProfile(const std::string &profileId,
                  rocklaunch::ConfigStore &configStore,
                  const rocklaunch::Rocksmith2014RemasteredProfile &gameProfile)
{
    if (!configStore.ProfileExists(profileId)) {
        rocklaunch::ProfileConfig config;
        config.id = profileId;
        config.gameId = gameProfile.Id();
        config.prefixDir = rocklaunch::ConfigStore::DefaultDataDir() / "prefixes" / profileId;
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
              << "Prefix path: " << config.prefixDir << '\n'
              << "Runner: " << (config.runnerId.empty() ? "not assigned" : config.runnerId) << '\n'
              << "Install path: ";
    if (config.installDir.empty()) {
        std::cout << "not assigned\n";
    } else {
        std::cout << config.installDir << '\n';
    }

    std::cout << "Applied patches: ";
    bool anyPatch = false;
    for (const auto &entry : config.patches) {
        if (entry.second.enabled) {
            if (anyPatch) {
                std::cout << ", ";
            }
            std::cout << entry.first;
            anyPatch = true;
        }
    }
    if (!anyPatch) {
        std::cout << "none";
    }
    std::cout << '\n';

    return 0;
}

// Runner commands

int ListRunners(const rocklaunch::RunnerManager &runnerManager)
{
    std::vector<rocklaunch::Runner> runners = runnerManager.List();
    if (runners.empty()) {
        std::cout << "No Wine or Proton runners were found.\n";
        return 0;
    }

    std::cout << std::left << std::setw(56) << "id" << std::setw(8) << "type"
              << std::setw(10) << "source" << "executable\n";
    for (const rocklaunch::Runner &runner : runners) {
        std::cout << std::setw(56) << runner.id << std::setw(8)
                  << rocklaunch::RunnerTypeName(runner.type) << std::setw(10) << runner.source
                  << runner.executable.string() << '\n';
    }

    return 0;
}

int SetRunner(const std::string &profileId,
               const std::string &runnerId,
               rocklaunch::ConfigStore &configStore,
               const rocklaunch::RunnerManager &runnerManager)
{
    if (!configStore.ProfileExists(profileId)) {
        std::cerr << "Profile not found: " << profileId << '\n';
        return 1;
    }

    if (!runnerManager.Find(runnerId).has_value()) {
        std::cerr << "Runner not found: " << runnerId << '\n';
        return 1;
    }

    rocklaunch::ProfileConfig config = configStore.LoadProfile(profileId);
    config.runnerId = runnerId;
    configStore.SaveProfile(config);
    std::cout << "Assigned runner " << runnerId << " to profile " << profileId << '\n';
    return 0;
}

int RemoveProfile(const std::string &profileId,
                  rocklaunch::ConfigStore &configStore,
                  bool force)
{
    if (!configStore.ProfileExists(profileId)) {
        std::cerr << "Profile not found: " << profileId << '\n';
        return 1;
    }

    if (!force && !ConfirmDestructive("Remove profile " + profileId + " and its prefix?")) {
        std::cout << "Aborted.\n";
        return 1;
    }

    if (!configStore.DeleteProfile(profileId)) {
        std::cerr << "Profile not found: " << profileId << '\n';
        return 1;
    }

    std::cout << "Removed profile: " << profileId << '\n';
    return 0;
}

// Patch commands

int PatchListAll(const rocklaunch::PatchManager &patchManager)
{
    std::cout << std::left << std::setw(26) << "Patch" << std::setw(26) << "Game" << "Name\n";
    for (const rocklaunch::ILaunchPatch *patch : patchManager.List()) {
        rocklaunch::PatchPreset preset = patch->Preset();
        std::cout << std::setw(26) << patch->Id() << std::setw(26) << preset.gameId
                  << preset.name << '\n';
    }

    return 0;
}

int PatchList(const std::string &profileId,
              rocklaunch::ConfigStore &configStore,
              const rocklaunch::PatchManager &patchManager)
{
    if (!configStore.ProfileExists(profileId)) {
        std::cerr << "Profile not found: " << profileId << '\n';
        return 1;
    }

    rocklaunch::ProfileConfig config = configStore.LoadProfile(profileId);
    std::cout << std::left << std::setw(26) << "Patch" << std::setw(26) << "Game"
              << std::setw(10) << "Status" << "Name\n";
    for (const rocklaunch::ILaunchPatch *patch : patchManager.List()) {
        if (patch->GameId() != config.gameId) {
            continue;
        }

        rocklaunch::PatchPreset preset = patch->Preset();
        std::cout << std::setw(26) << patch->Id() << std::setw(26) << preset.gameId
                  << std::setw(10)
                  << (patch->IsEnabled(config) ? "enabled" : "disabled") << preset.name << '\n';
    }

    return 0;
}

int PatchAdd(const std::string &profileId,
             const std::string &patchId,
             rocklaunch::PatchManager &patchManager,
             bool force)
{
    std::string error;
    if (!patchManager.Enable(profileId, patchId, error, force)) {
        std::cerr << error << '\n';
        return 1;
    }

    std::cout << "Enabled patch " << patchId << " on profile " << profileId << '\n';
    return 0;
}

int PatchRemove(const std::string &profileId,
                const std::string &patchId,
                rocklaunch::PatchManager &patchManager,
                bool force)
{
    std::string error;
    if (!patchManager.Disable(profileId, patchId, error, force)) {
        std::cerr << error << '\n';
        return 1;
    }

    std::cout << "Disabled patch " << patchId << " on profile " << profileId << '\n';
    return 0;
}

int PatchStatus(const std::string &profileId,
                const std::string &patchId,
                rocklaunch::ConfigStore &configStore,
                const rocklaunch::PatchManager &patchManager)
{
    if (!configStore.ProfileExists(profileId)) {
        std::cerr << "Profile not found: " << profileId << '\n';
        return 1;
    }

    const rocklaunch::ILaunchPatch *patch = patchManager.Find(patchId);
    if (patch == nullptr) {
        std::cerr << "Unknown patch: " << patchId << '\n';
        return 1;
    }

    rocklaunch::ProfileConfig config = configStore.LoadProfile(profileId);
    rocklaunch::PatchPreset preset = patch->Preset();
    std::cout << "Patch: " << patch->Id() << '\n'
              << "Name: " << preset.name << '\n'
              << "Game: " << preset.gameId << '\n'
              << "Status: " << (patch->IsEnabled(config) ? "enabled" : "disabled") << '\n'
              << "Reversible: " << (preset.reversible ? "yes" : "no") << '\n'
              << "Install-level: " << (preset.installLevel ? "yes" : "no") << '\n'
              << "Description: " << preset.description << '\n';
    for (const rocklaunch::PatchOperation &operation : preset.operations) {
        std::cout << "  " << rocklaunch::PatchOperationTypeName(operation.type) << ' '
                  << operation.target << " - " << operation.detail << '\n';
    }

    return 0;
}

// Path assignment

int SetPath(const std::string &profileId,
            const char *path,
            rocklaunch::ConfigStore &configStore,
            const rocklaunch::Rocksmith2014RemasteredProfile &gameProfile)
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

    // A profile is bound to one game; never point it at another game's installation.
    if (config.gameId != gameProfile.Id()) {
        std::cerr << "Profile " << profileId << " is for game " << config.gameId
                  << ", not " << gameProfile.Id() << '\n';
        return 1;
    }

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

// Launch

int LaunchProfile(const std::string &profileId,
                  rocklaunch::ConfigStore &configStore,
                  const rocklaunch::Rocksmith2014RemasteredProfile &gameProfile,
                  const rocklaunch::RunnerManager &runnerManager)
{
    if (!configStore.ProfileExists(profileId)) {
        std::cerr << "Profile not found: " << profileId << '\n';
        return 1;
    }

    rocklaunch::ProfileConfig config = configStore.LoadProfile(profileId);

    // The game the profile belongs to must match the game we are about to launch.
    if (config.gameId != gameProfile.Id()) {
        std::cerr << "Profile " << profileId << " is for game " << config.gameId
                  << ", which this build does not support.\n";
        return 1;
    }

    if (config.installDir.empty()) {
        std::cerr << "Profile " << profileId << " has no install path. "
                  << "Use set-path <profile> <path> first.\n";
        return 1;
    }

    if (config.runnerId.empty()) {
        std::cerr << "Profile " << profileId << " has no runner. "
                  << "Use runner set <profile> <runner> first.\n";
        return 1;
    }

    std::optional<rocklaunch::Runner> runner = runnerManager.Find(config.runnerId);
    if (!runner.has_value()) {
        std::cerr << "Runner not found: " << config.runnerId << '\n';
        return 1;
    }

    rocklaunch::LaunchCommand launch = rocklaunch::BuildLaunchCommand(config, *runner, gameProfile);

    std::vector<std::string> warnings = rocklaunch::EnsurePrefix(config.prefixDir, *runner);
    for (const std::string &warning : warnings) {
        std::cerr << "Warning: " << warning << '\n';
    }

    if (!rocklaunch::ExecLaunchCommand(launch)) {
        std::cerr << "Failed to start '" << launch.command.front() << "': "
                  << std::strerror(errno) << '\n';
        return 1;
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
        rocklaunch::Rocksmith2014RemasteredProfile profile;
        rocklaunch::RunnerManager runnerManager = rocklaunch::RunnerManager::CreateDefault();
        rocklaunch::PatchManager patchManager = rocklaunch::PatchManager::CreateDefault(configStore);

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

        if (argument == "runner" && argc == 3 && std::string_view(argv[2]) == "list") {
            logger.Info("Listing available runners");
            return ListRunners(runnerManager);
        }

        if (argument == "runner" && argc == 5 && std::string_view(argv[2]) == "set") {
            logger.Info("Assigning a runner to a profile");
            return SetRunner(argv[3], argv[4], configStore, runnerManager);
        }

        if (argument == "launch" && argc == 3) {
            logger.Info("Launching a profile");
            return LaunchProfile(argv[2], configStore, profile, runnerManager);
        }

        if (argument == "patch" && argc == 3 && std::string_view(argv[2]) == "list") {
            logger.Info("Listing all available patches");
            return PatchListAll(patchManager);
        }

        if (argument == "patch" && argc == 4 && std::string_view(argv[2]) == "list") {
            logger.Info("Listing patches for a profile");
            return PatchList(argv[3], configStore, patchManager);
        }

        if (argument == "patch" && argc == 5 && std::string_view(argv[2]) == "add") {
            logger.Info("Enabling a patch on a profile");
            return PatchAdd(argv[3], argv[4], patchManager, false);
        }

        if (argument == "patch" && argc == 6 && std::string_view(argv[2]) == "add"
            && (std::string_view(argv[3]) == "-f"
                || std::string_view(argv[3]) == "--force")) {
            logger.Info("Enabling a patch on a profile (forced)");
            return PatchAdd(argv[4], argv[5], patchManager, true);
        }

        if (argument == "patch" && argc == 5 && std::string_view(argv[2]) == "remove") {
            logger.Info("Disabling a patch on a profile");
            return PatchRemove(argv[3], argv[4], patchManager, false);
        }

        if (argument == "patch" && argc == 6 && std::string_view(argv[2]) == "remove"
            && (std::string_view(argv[3]) == "-f"
                || std::string_view(argv[3]) == "--force")) {
            logger.Info("Disabling a patch on a profile (forced)");
            return PatchRemove(argv[4], argv[5], patchManager, true);
        }

        if (argument == "patch" && argc == 5 && std::string_view(argv[2]) == "status") {
            logger.Info("Showing patch status");
            return PatchStatus(argv[3], argv[4], configStore, patchManager);
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

        if (argument == "profile" && argc == 4 && std::string_view(argv[2]) == "remove") {
            logger.Info("Deleting a profile configuration");
            return RemoveProfile(argv[3], configStore, false);
        }

        if (argument == "profile" && argc == 5 && std::string_view(argv[2]) == "remove"
            && (std::string_view(argv[3]) == "-f"
                || std::string_view(argv[3]) == "--force")) {
            logger.Info("Deleting a profile configuration (forced)");
            return RemoveProfile(argv[4], configStore, true);
        }

        std::cerr << "Unknown option: " << argument << '\n';
        PrintUsage();
        return 1;
    } catch (const std::exception &error) {
        std::cerr << "rocklaunch-cli: " << error.what() << '\n';
        return 1;
    }
}
