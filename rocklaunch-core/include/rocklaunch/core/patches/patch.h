#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace rocklaunch
{

namespace fs = std::filesystem;

// The kind of change a patch makes to an installation.
enum class PatchOperationType
{
    EditFile,     // modify an entry inside a packed data file (e.g. cache.psarc)
    EditIni,      // set a key inside an INI file (creates section/file if missing)
    CopyFile,     // place a file into the game directory
    RemoveFile,   // delete a file from the game directory
    RestoreFile,  // restore a previously backed-up original
    SetEnv,       // environment variable only, no on-disk change
};

// One step of a patch's plan, described for `patch status` and the future GUI.
struct PatchOperation
{
    PatchOperationType type;
    fs::path target;   // path relative to the install dir
    std::string detail;
};

// Human-readable description of what a patch does and what it touches. Used by
// `patch list/status`; the actual work happens in ILaunchPatch::Apply/Remove.
struct PatchPreset
{
    std::string id;
    std::string gameId;       // game this patch was designed for
    std::string name;
    std::string description;
    bool reversible;          // backup/restore supported
    bool installLevel;        // persistent on-disk change (vs per-launch)
    std::vector<PatchOperation> operations;
};

std::string PatchOperationTypeName(PatchOperationType type);

} // namespace rocklaunch
