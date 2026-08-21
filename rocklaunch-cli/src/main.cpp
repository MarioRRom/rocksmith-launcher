#include "cli_ui.h"

#include "rocklaunch/core/config_store.h"
#include "rocklaunch/core/launch.h"
#include "rocklaunch/core/manual_source.h"
#include "rocklaunch/core/patches/patch_manager.h"
#include "rocklaunch/core/rocksmith2014_remastered_profile.h"
#include "rocklaunch/core/runners/launcher_runner_source.h"
#include "rocklaunch/core/runners/runner_manager.h"
#include "rocklaunch/core/utils/string_util.h"

#include <cerrno>
#include <cstring>
#include <exception>
#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace
{

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

    PrintError("Profile already exists: " + profileId);
    return 1;
}

int ShowProfile(const std::string &profileId, rocklaunch::ConfigStore &configStore)
{
    if (!configStore.ProfileExists(profileId)) {
        PrintError("Profile not found: " + profileId);
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

    std::cout << Color(PadLeft("Id", 56), "1") << Color(PadLeft("Type", 8), "1")
              << Color(PadLeft("Source", 10), "1") << Color("Executable", "1") << '\n';
    for (const rocklaunch::Runner &runner : runners) {
        std::cout << Color(PadLeft(runner.id, 56), kRunnerColor)
                  << PadLeft(rocklaunch::RunnerTypeName(runner.type), 8)
                  << PadLeft(runner.source, 10) << runner.executable.string() << '\n';
    }

    return 0;
}

int SetRunner(const std::string &profileId,
               const std::string &runnerId,
               rocklaunch::ConfigStore &configStore,
               const rocklaunch::RunnerManager &runnerManager)
{
    if (!configStore.ProfileExists(profileId)) {
        PrintError("Profile not found: " + profileId);
        return 1;
    }

    if (!runnerManager.Find(runnerId).has_value()) {
        PrintError("Runner not found: " + runnerId);
        return 1;
    }

    rocklaunch::ProfileConfig config = configStore.LoadProfile(profileId);
    config.runnerId = runnerId;
    configStore.SaveProfile(config);
    std::cout << "Assigned runner " << runnerId << " to profile " << profileId << '\n';
    return 0;
}

int UpdateRunnerList(rocklaunch::ConfigStore &configStore)
{
    rocklaunch::RunnerManager runnerManager = rocklaunch::RunnerManager::CreateDefault();
    runnerManager.Search("", configStore.DataDir(), true);
    std::cout << "Updated the releases list.\n";
    return 0;
}

int SearchRunners(const std::string &query,
                  rocklaunch::ConfigStore &configStore,
                  bool forceRefresh)
{
    rocklaunch::RunnerManager runnerManager = rocklaunch::RunnerManager::CreateDefault();
    std::vector<rocklaunch::RunnerRelease> releases =
        runnerManager.Search(query, configStore.DataDir(), forceRefresh);

    if (releases.empty()) {
        std::cout << "No releases found for: " << query << '\n';
        return 0;
    }

    std::vector<rocklaunch::Runner> installed = runnerManager.List();
    std::set<std::string> installedNames;
    for (const rocklaunch::Runner &r : installed) {
        installedNames.insert(rocklaunch::ToLower(r.name));
    }

    for (const rocklaunch::RunnerRelease &rr : releases) {
        std::string size = "?";
        std::optional<rocklaunch::AssetInfo> asset = runnerManager.SelectAsset(rr);
        if (asset.has_value()) {
            size = HumanSize(asset->size);
        }

        std::string package = RepoShortName(rr.repo) + '/' + Color(rr.tag, kRunnerColor);

        std::cout << package << "  " << size;
        if (installedNames.count(rocklaunch::ToLower(rr.tag)) > 0) {
            std::cout << "  " << Color("[installed]", "32");
        }
        std::cout << '\n';
    }

    return 0;
}

int InstallRunner(const std::string &runnerName,
                  const std::string &assetName,
                  bool force,
                  rocklaunch::ConfigStore &configStore)
{
    rocklaunch::fs::path runnersDir = rocklaunch::LauncherRunnerSource::DefaultRunnerDir();
    rocklaunch::RunnerManager runnerManager = rocklaunch::RunnerManager::CreateDefault();

    // Resolve the canonical tag (case-insensitive) so the prompt and folder
    // name match what upstream uses, regardless of how the user typed it.
    std::string canonicalName = runnerName;
    std::optional<std::string> resolved =
        runnerManager.ResolveName(runnerName, configStore.DataDir());
    if (resolved.has_value()) {
        canonicalName = *resolved;
    } else {
        PrintError("No release named '" + runnerName + "' in cache. "
                   "Use 'runner search <query>' to browse available releases.");
        return 1;
    }

    if (!force) {
        bool alreadyInstalled = runnerManager.IsInstalled(canonicalName, runnersDir);
        std::string prompt = alreadyInstalled
            ? "Runner " + canonicalName + " is already installed. Reinstall it?"
            : "Install runner " + canonicalName + "?";
        if (!ConfirmDestructive(prompt)) {
            std::cout << "Aborted.\n";
            return 1;
        }
    }

    try {
        runnerManager.Install(runnerName, assetName, runnersDir);
    } catch (const std::exception &error) {
        PrintError("Install failed: " + std::string(error.what()));
        return 1;
    }

    std::cout << "Installed runner: " << canonicalName << '\n';
    return 0;
}

int RemoveRunner(const std::string &runnerName, bool force,
                 rocklaunch::ConfigStore &configStore)
{
    rocklaunch::fs::path runnersDir = rocklaunch::LauncherRunnerSource::DefaultRunnerDir();
    rocklaunch::RunnerManager runnerManager = rocklaunch::RunnerManager::CreateDefault();

    std::string canonicalName = runnerName;
    std::optional<std::string> resolved =
        runnerManager.ResolveName(runnerName, configStore.DataDir());
    if (resolved.has_value()) {
        canonicalName = *resolved;
    } else {
        PrintError("No release named '" + runnerName + "' in cache. "
                   "Use 'runner search <query>' to browse available releases.");
        return 1;
    }

    if (!force) {
        std::string prompt = "Remove runner " + canonicalName + "?";
        if (!ConfirmDestructive(prompt)) {
            std::cout << "Aborted.\n";
            return 1;
        }
    }

    try {
        runnerManager.Remove(canonicalName, runnersDir);
    } catch (const std::exception &error) {
        PrintError("Remove failed: " + std::string(error.what()));
        return 1;
    }

    std::cout << "Removed runner: " << canonicalName << '\n';
    return 0;
}

int RemoveProfile(const std::string &profileId,
                  rocklaunch::ConfigStore &configStore,
                  bool force)
{
    if (!configStore.ProfileExists(profileId)) {
        PrintError("Profile not found: " + profileId);
        return 1;
    }

    if (!force && !ConfirmDestructive("Remove profile " + profileId + " and its prefix?")) {
        std::cout << "Aborted.\n";
        return 1;
    }

    if (!configStore.DeleteProfile(profileId)) {
        PrintError("Profile not found: " + profileId);
        return 1;
    }

    std::cout << "Removed profile: " << profileId << '\n';
    return 0;
}

// Patch commands

int PatchListAll(const rocklaunch::PatchManager &patchManager)
{
    std::cout << Color(PadLeft("Patch", 26), "1") << Color(PadLeft("Game", 26), "1")
              << Color("Name", "1") << '\n';
    for (const rocklaunch::ILaunchPatch *patch : patchManager.List()) {
        rocklaunch::PatchPreset preset = patch->Preset();
        std::cout << Color(PadLeft(patch->Id(), 26), kPatchColor)
                  << PadLeft(preset.gameId, 26) << preset.name << '\n';
    }

    return 0;
}

int PatchList(const std::string &profileId,
              rocklaunch::ConfigStore &configStore,
              const rocklaunch::PatchManager &patchManager)
{
    if (!configStore.ProfileExists(profileId)) {
        PrintError("Profile not found: " + profileId);
        return 1;
    }

    rocklaunch::ProfileConfig config = configStore.LoadProfile(profileId);
    std::cout << Color(PadLeft("Patch", 26), "1") << Color(PadLeft("Game", 26), "1")
              << Color(PadLeft("Status", 10), "1") << Color("Name", "1") << '\n';
    for (const rocklaunch::ILaunchPatch *patch : patchManager.List()) {
        if (patch->GameId() != config.gameId) {
            continue;
        }

        rocklaunch::PatchPreset preset = patch->Preset();
        bool enabled = patch->IsEnabled(config);
        std::string status = enabled ? "enabled" : "disabled";
        std::cout << Color(PadLeft(patch->Id(), 26), kPatchColor) << PadLeft(preset.gameId, 26)
                  << Color(PadLeft(status, 10), enabled ? "32" : kPatchColor)
                  << preset.name << '\n';
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
        PrintError(error);
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
        PrintError(error);
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
        PrintError("Profile not found: " + profileId);
        return 1;
    }

    const rocklaunch::ILaunchPatch *patch = patchManager.Find(patchId);
    if (patch == nullptr) {
        PrintError("Unknown patch: " + patchId);
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
            const std::string &path,
            rocklaunch::ConfigStore &configStore,
            const rocklaunch::Rocksmith2014RemasteredProfile &gameProfile)
{
    if (!configStore.ProfileExists(profileId)) {
        PrintError("Profile not found: " + profileId
                    + "\nCreate it first with profile new <profile>.");
        return 1;
    }

    rocklaunch::ManualSource source(path);
    std::optional<rocklaunch::fs::path> installDir = source.Locate(gameProfile);
    if (!installDir.has_value()) {
        PrintError("Invalid Rocksmith 2014 installation: " + path
                    + "\nExpected Rocksmith2014.exe and a dlc directory.");
        return 1;
    }

    std::optional<std::string> conflictingProfile =
        configStore.ProfileUsingInstallDir(*installDir, profileId);
    if (conflictingProfile.has_value()) {
        PrintError("This game installation is already used by profile: "
                    + *conflictingProfile);
        return 1;
    }

    rocklaunch::ProfileConfig config = configStore.LoadProfile(profileId);

    // A profile is bound to one game; never point it at another game's installation.
    if (config.gameId != gameProfile.Id()) {
        PrintError("Profile " + profileId + " is for game " + config.gameId
                    + ", not " + gameProfile.Id());
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
        std::cout << Color(profileId, kProfileColor) << '\n';
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
        PrintError("Profile not found: " + profileId);
        return 1;
    }

    rocklaunch::ProfileConfig config = configStore.LoadProfile(profileId);

    // The game the profile belongs to must match the game we are about to launch.
    if (config.gameId != gameProfile.Id()) {
        PrintError("Profile " + profileId + " is for game " + config.gameId
                    + ", which this build does not support.");
        return 1;
    }

    if (config.installDir.empty()) {
        PrintError("Profile " + profileId + " has no install path. "
                    + "Use set-path <profile> <path> first.");
        return 1;
    }

    if (config.runnerId.empty()) {
        PrintError("Profile " + profileId + " has no runner. "
                    + "Use runner set <profile> <runner> first.");
        return 1;
    }

    std::optional<rocklaunch::Runner> runner = runnerManager.Find(config.runnerId);
    if (!runner.has_value()) {
        PrintError("Runner not found: " + config.runnerId);
        return 1;
    }

    rocklaunch::LaunchCommand launch = rocklaunch::BuildLaunchCommand(config, *runner, gameProfile);

    std::vector<std::string> warnings = rocklaunch::EnsurePrefix(config.prefixDir, *runner);
    for (const std::string &warning : warnings) {
        PrintWarning("Warning: " + warning);
    }

    if (!rocklaunch::ExecLaunchCommand(launch)) {
        PrintError("Failed to start '" + launch.command.front() + "': "
                    + std::strerror(errno));
        return 1;
    }

    return 0;
}

} // namespace

// Entry point

int main(int argc, char *argv[])
{
    try {
        rocklaunch::ConfigStore configStore;
        rocklaunch::Rocksmith2014RemasteredProfile profile;
        rocklaunch::RunnerManager runnerManager = rocklaunch::RunnerManager::CreateDefault();
        rocklaunch::PatchManager patchManager =
            rocklaunch::PatchManager::CreateDefault(configStore);

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
            return SetPath(argv[2], argv[3], configStore, profile);
        }

        if (argument == "runner" && argc == 3 && std::string_view(argv[2]) == "-u") {
            return UpdateRunnerList(configStore);
        }

        if (argument == "runner" && argc == 3 && std::string_view(argv[2]) == "list") {
            return ListRunners(runnerManager);
        }

        if (argument == "runner" && argc == 5 && std::string_view(argv[2]) == "set") {
            return SetRunner(argv[3], argv[4], configStore, runnerManager);
        }

        if (argument == "runner" && argc >= 4 && std::string_view(argv[2]) == "search") {
            bool refresh = false;
            std::string query;
            for (int i = 3; i < argc; ++i) {
                if (std::string_view(argv[i]) == "-u") {
                    refresh = true;
                } else {
                    if (!query.empty()) {
                        query += ' ';
                    }
                    query += argv[i];
                }
            }
            return SearchRunners(query, configStore, refresh);
        }

        if (argument == "runner" && argc >= 4 && std::string_view(argv[2]) == "install") {
            std::string runnerName = argv[3];
            std::string assetName;
            bool force = false;
            for (int i = 4; i < argc; ++i) {
                if (IsForceFlag(std::string_view(argv[i]))) {
                    force = true;
                } else if (assetName.empty()) {
                    assetName = argv[i];
                }
            }
            return InstallRunner(runnerName, assetName, force, configStore);
        }

        if (argument == "runner" && argc >= 4 && std::string_view(argv[2]) == "remove") {
            bool force = false;
            std::string runnerName;
            for (int i = 3; i < argc; ++i) {
                if (IsForceFlag(std::string_view(argv[i]))) {
                    force = true;
                } else if (runnerName.empty()) {
                    runnerName = argv[i];
                }
            }
            return RemoveRunner(runnerName, force, configStore);
        }

        if (argument == "launch" && argc == 3) {
            return LaunchProfile(argv[2], configStore, profile, runnerManager);
        }

        if (argument == "patch" && argc == 3 && std::string_view(argv[2]) == "list") {
            return PatchListAll(patchManager);
        }

        if (argument == "patch" && argc == 4 && std::string_view(argv[2]) == "list") {
            return PatchList(argv[3], configStore, patchManager);
        }

        if (argument == "patch" && argc == 5 && std::string_view(argv[2]) == "add") {
            return PatchAdd(argv[3], argv[4], patchManager, false);
        }

        if (argument == "patch" && argc == 6 && std::string_view(argv[2]) == "add"
            && IsForceFlag(std::string_view(argv[3]))) {
            return PatchAdd(argv[4], argv[5], patchManager, true);
        }

        if (argument == "patch" && argc == 5 && std::string_view(argv[2]) == "remove") {
            return PatchRemove(argv[3], argv[4], patchManager, false);
        }

        if (argument == "patch" && argc == 6 && std::string_view(argv[2]) == "remove"
            && IsForceFlag(std::string_view(argv[3]))) {
            return PatchRemove(argv[4], argv[5], patchManager, true);
        }

        if (argument == "patch" && argc == 5 && std::string_view(argv[2]) == "status") {
            return PatchStatus(argv[3], argv[4], configStore, patchManager);
        }

        if (argument == "profile" && argc == 3 && std::string_view(argv[2]) == "list") {
            return ListProfiles(configStore);
        }

        if (argument == "profile" && argc == 3 && std::string_view(argv[2]) == "new") {
            std::string profileId = NextDefaultProfileId(profile.Id(), configStore);
            return CreateProfile(profileId, configStore, profile);
        }

        if (argument == "profile" && argc == 4 && std::string_view(argv[2]) == "new") {
            return CreateProfile(argv[3], configStore, profile);
        }

        if (argument == "profile" && argc == 4 && std::string_view(argv[2]) == "show") {
            return ShowProfile(argv[3], configStore);
        }

        if (argument == "profile" && argc == 4 && std::string_view(argv[2]) == "remove") {
            return RemoveProfile(argv[3], configStore, false);
        }

        if (argument == "profile" && argc == 5 && std::string_view(argv[2]) == "remove"
            && IsForceFlag(std::string_view(argv[3]))) {
            return RemoveProfile(argv[4], configStore, true);
        }

        bool knownTopLevel = argument == "profile" || argument == "runner"
            || argument == "patch" || argument == "launch"
            || argument == "set-path";
        if (knownTopLevel) {
            std::string subcommand = argc >= 3 ? argv[2] : "";
            if (!subcommand.empty() && !IsKnownSubcommand(argument, argv[2])) {
                PrintUsageError("unrecognized subcommand '" + subcommand + "'",
                                std::string(argument));
            } else {
                PrintUsageError("invalid arguments for '" + std::string(argument) + "'",
                                std::string(argument), subcommand);
            }
            return 1;
        }

        PrintUsageError("unrecognized subcommand '" + std::string(argument) + "'",
                        std::string(argument));
        return 1;
    } catch (const std::exception &error) {
        PrintError("rocklaunch-cli: " + std::string(error.what()));
        return 1;
    }
}
