#include "rocklaunch/core/patches/direct_connect_patch.h"

#include "rocklaunch/core/logger.h"
#include "rocklaunch/core/psarc_util.h"
#include "rocklaunch/core/subprocess.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace rocklaunch
{

namespace
{

// Fixes the game's non-standard JSON: missing commas between sibling entries
// and trailing commas before } or ]. nlohmann::json can't parse either.
// Two-pass approach: first insert missing commas, then strip trailing ones.
std::string FixGameJsonSyntax(const std::string &raw)
{
    // Pass 1: insert missing commas between } or ] and the next opening ".
    std::string step1;
    step1.reserve(raw.size());
    bool inStr = false;
    bool esc = false;

    for (size_t i = 0; i < raw.size(); ++i) {
        char c = raw[i];
        if (esc) {
            step1 += c;
            esc = false;
            continue;
        }
        if (inStr) {
            if (c == '\\') {
                esc = true;
            } else if (c == '"') {
                inStr = false;
            }
            step1 += c;
            continue;
        }
        if (c == '"') {
            // If previous non-whitespace is } or ], insert comma first.
            size_t j = step1.size();
            while (j > 0 && (step1[j - 1] == ' ' || step1[j - 1] == '\t'
                             || step1[j - 1] == '\n' || step1[j - 1] == '\r')) {
                --j;
            }
            if (j > 0 && (step1[j - 1] == '}' || step1[j - 1] == ']')) {
                step1 += ',';
            }
            inStr = true;
        }

        step1 += c;
    }

    // Pass 2: strip trailing commas before } or ].
    std::string step2;
    step2.reserve(step1.size());
    inStr = false;
    esc = false;

    for (size_t i = 0; i < step1.size(); ++i) {
        char c = step1[i];
        if (esc) {
            step2 += c;
            esc = false;
            continue;
        }
        if (inStr) {
            if (c == '\\') {
                esc = true;
            } else if (c == '"') {
                inStr = false;
            }
            step2 += c;
            continue;
        }
        if (c == '"') {
            inStr = true;
            step2 += c;
            continue;
        }
        if (c == ',') {
            size_t j = i + 1;
            while (j < step1.size() && (step1[j] == ' ' || step1[j] == '\t'
                                        || step1[j] == '\n' || step1[j] == '\r')) {
                ++j;
            }
            if (j < step1.size() && (step1[j] == '}' || step1[j] == ']')) {
                continue; // drop trailing comma
            }
        }
        step2 += c;
    }

    return step2;
}

nlohmann::json ParseGameJson(const fs::path &path)
{
    std::ifstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("Cannot open JSON file: " + path.string());
    }

    std::string raw((std::istreambuf_iterator<char>(f)),
                     std::istreambuf_iterator<char>());
    f.close();

    return nlohmann::json::parse(FixGameJsonSyntax(raw));
}

void WriteGameJson(const fs::path &path, const nlohmann::json &root)
{
    std::ofstream outFile(path);
    if (!outFile.is_open()) {
        throw std::runtime_error("Cannot write JSON file: " + path.string());
    }

    // Use dump(4) for readability, matching the original game format.
    outFile << root.dump(4) << '\n';
}

void CopyFile(const fs::path &src, const fs::path &dst)
{
    std::ifstream in(src, std::ios::binary);
    if (!in.is_open()) {
        throw std::runtime_error("Cannot open source file: " + src.string());
    }

    // Write to a temp file in the same directory, then rename for atomicity.
    fs::path tmpPath = dst.parent_path() / (dst.filename().string() + ".tmp");
    std::ofstream out(tmpPath, std::ios::binary);
    if (!out.is_open()) {
        throw std::runtime_error("Cannot create destination file: " + tmpPath.string());
    }

    out << in.rdbuf();
    out.close();
    in.close();

    std::error_code ec;
    fs::rename(tmpPath, dst, ec);
    if (ec) {
        throw std::runtime_error("Failed to rename " + tmpPath.string()
                                 + " to " + dst.string() + ": " + ec.message());
    }
}

// --- JSON query helpers ---

bool StartupHasOthercable(const fs::path &path)
{
    auto root = ParseGameJson(path);

    auto &def = root["Static"]["UI"]["Menus"]["Entries"]["FE_InputSelect"]
                           ["View"]["Definition"];
    return def["Images"].contains("othercable")
        && def["Buttons"].contains("othercable");
}

bool MissionHasDirectConnect(const fs::path &path)
{
    auto root = ParseGameJson(path);

    auto &buttons = root["Static"]["UI"]["Menus"]["Entries"]["MissionMenu"]
                                ["View"]["Definition"]["Buttons"];
    return buttons["inputMode"]["AcceptedValues"].contains("3");
}

// --- JSON mutation helpers ---

void AddStartupOthercable(const fs::path &path)
{
    auto root = ParseGameJson(path);

    auto &def = root["Static"]["UI"]["Menus"]["Entries"]["FE_InputSelect"]
                           ["View"]["Definition"];

    auto &images = def["Images"];
    if (!images.contains("othercable")) {
        images["othercable"] = {
            {"ID", "othercable"},
            {"File", "inputmode_directconnect.png"},
            {"InputModeIconFileText", "Directcon_"}
        };
    }

    auto &buttons = def["Buttons"];
    if (!buttons.contains("othercable")) {
        buttons["othercable"] = {
            {"ID", "othercable"},
            {"Label", "DIRECT CONNECT"},
            {"Description",
             "$[37291]Use a cable other than the Real Tone Cable to plug in "
             "your electric guitar or bass."},
            {"State", "up"},
            {"SortOrder", 1}
        };
    }

    WriteGameJson(path, root);
}

void RemoveStartupOthercable(const fs::path &path)
{
    auto root = ParseGameJson(path);

    auto &def = root["Static"]["UI"]["Menus"]["Entries"]["FE_InputSelect"]
                           ["View"]["Definition"];

    def["Images"].erase("othercable");
    def["Buttons"].erase("othercable");

    WriteGameJson(path, root);
}

void AddMissionDirectConnect(const fs::path &path)
{
    auto root = ParseGameJson(path);

    auto &inputMode = root["Static"]["UI"]["Menus"]["Entries"]["MissionMenu"]
                                  ["View"]["Definition"]["Buttons"]
                                  ["inputMode"]["AcceptedValues"];

    if (!inputMode.contains("3")) {
        inputMode["3"] = "DIRECT CONNECT";
    }

    WriteGameJson(path, root);
}

void RemoveMissionDirectConnect(const fs::path &path)
{
    auto root = ParseGameJson(path);

    auto &inputMode = root["Static"]["UI"]["Menus"]["Entries"]["MissionMenu"]
                                  ["View"]["Definition"]["Buttons"]
                                  ["inputMode"]["AcceptedValues"];

    inputMode.erase("3");

    WriteGameJson(path, root);
}

// Core operation: extract PSARC → edit 7z in-place → repack PSARC.
// patchDirection: true = add entries, false = remove entries.
// Returns true if any change was made, false if the state was already correct.
bool PatchCachePsarc(const fs::path &gameCache, bool patchDirection)
{
    Logger logger;
    fs::path tmpDir = fs::temp_directory_path() / "rocksmith-launcher";

    std::error_code ec;
    fs::remove_all(tmpDir, ec);
    fs::create_directories(tmpDir);

    // Backup cache.psarc next to the original (first time only).
    fs::path bakPath = gameCache.parent_path() / "cache.psarc.bak";
    if (!fs::exists(bakPath)) {
        CopyFile(gameCache, bakPath);
        logger.Info("DirectConnectPatch: backup created at " + bakPath.string());
    }

    fs::path psarcTmp = tmpDir / "psarc";
    fs::create_directories(psarcTmp);
    psarc_util::Extract(gameCache, psarcTmp);

    fs::path cache7z = psarcTmp / "cache7.7z";
    if (!fs::exists(cache7z)) {
        throw std::runtime_error("cache7.7z not found in extracted PSARC");
    }

    RunSubprocess({"7z", "x", cache7z.string(),
                   "manifests/ui_menu_pillar_startup.database.json",
                   "manifests/ui_menu_pillar_mission.database.json",
                   "-o" + tmpDir.string(), "-y"});

    fs::path manifestsDir = tmpDir / "manifests";
    fs::path startupJson = manifestsDir / "ui_menu_pillar_startup.database.json";
    fs::path missionJson = manifestsDir / "ui_menu_pillar_mission.database.json";

    bool startupPresent = StartupHasOthercable(startupJson);
    bool missionPresent = MissionHasDirectConnect(missionJson);

    bool alreadyInTargetState = patchDirection
        ? (startupPresent && missionPresent)
        : (!startupPresent && !missionPresent);

    if (alreadyInTargetState) {
        fs::remove_all(tmpDir, ec);
        return false;
    }

    if (patchDirection) {
        AddStartupOthercable(startupJson);
        AddMissionDirectConnect(missionJson);
    } else {
        RemoveStartupOthercable(startupJson);
        RemoveMissionDirectConnect(missionJson);
    }

    RunSubprocess({"7z", "u", cache7z.string(),
                   "manifests/ui_menu_pillar_startup.database.json",
                   "manifests/ui_menu_pillar_mission.database.json"},
                  tmpDir);

    // Copy+remove instead of rename — temp dir and game install may be on
    // different filesystems.
    fs::path repackedTmp = tmpDir / "cache.psarc.repacked";
    psarc_util::Repack(psarcTmp, repackedTmp);

    fs::remove(gameCache);
    CopyFile(repackedTmp, gameCache);
    fs::remove(repackedTmp);

    fs::remove_all(tmpDir, ec);

    return true;
}

} // namespace

std::string DirectConnectPatch::Id() const
{
    return "direct-connect";
}

std::string DirectConnectPatch::GameId() const
{
    return "rocksmith2014remastered";
}

PatchPreset DirectConnectPatch::Preset() const
{
    return {
        Id(),
        GameId(),
        "Direct Connect Mode",
        "Enables the game's hidden Input > Direct Connect mode so any audio "
        "device can be used as the guitar input. Edits cache.psarc in-place; "
        "original preserved as cache.psarc.bak for safe revert.",
        true,
        true,
        {
            { PatchOperationType::EditFile, fs::path("cache.psarc"),
              "toggle the Direct Connect flag inside cache7.7z manifests" },
            { PatchOperationType::RestoreFile, fs::path("cache.psarc"),
              "restore the backed-up original when disabled" },
        },
    };
}

bool DirectConnectPatch::IsEnabled(const ProfileConfig &profile) const
{
    const auto it = profile.patches.find(Id());
    return it != profile.patches.end() && it->second.enabled;
}

void DirectConnectPatch::Apply(const ProfileConfig &profile, bool force) const
{
    if (profile.gameId != GameId()) {
        throw std::runtime_error("Patch " + Id() + " is for game " + GameId()
                                 + ", profile is for game " + profile.gameId);
    }

    fs::path gameCache = profile.installDir / "cache.psarc";
    if (!fs::exists(gameCache)) {
        throw std::runtime_error("cache.psarc not found at: " + gameCache.string());
    }

    PatchCachePsarc(gameCache, true);
}

void DirectConnectPatch::Remove(const ProfileConfig &profile) const
{
    if (profile.gameId != GameId()) {
        throw std::runtime_error("Patch " + Id() + " is for game " + GameId()
                                 + ", profile is for game " + profile.gameId);
    }

    fs::path gameCache = profile.installDir / "cache.psarc";
    if (!fs::exists(gameCache)) {
        throw std::runtime_error("cache.psarc not found at: " + gameCache.string());
    }

    PatchCachePsarc(gameCache, false);
}

} // namespace rocklaunch
