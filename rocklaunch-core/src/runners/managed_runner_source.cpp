#include "rocklaunch/core/runners/managed_runner_source.h"

#include <cstdlib>
#include <stdexcept>
#include <utility>

namespace rocklaunch
{

ManagedRunnerSource::ManagedRunnerSource(fs::path runnerDir)
    : m_runnerDir(std::move(runnerDir))
{
}

std::vector<Runner> ManagedRunnerSource::Discover() const
{
    std::vector<Runner> runners;
    std::error_code error;
    if (!fs::is_directory(m_runnerDir, error)) {
        return runners;
    }

    for (const fs::directory_entry &entry : fs::directory_iterator(m_runnerDir)) {
        fs::path proton = entry.path() / "proton";
        if (entry.is_directory() && fs::is_regular_file(proton, error)) {
            std::string name = entry.path().filename().string();
            runners.push_back({ "managed-" + SanitizeRunnerId(name), name,
                                 RunnerType::Proton, "managed", entry.path(), proton });
        }
    }

    return runners;
}

fs::path ManagedRunnerSource::DefaultRunnerDir()
{
    const char *dataHome = std::getenv("XDG_DATA_HOME");
    if (dataHome != nullptr) {
        return fs::path(dataHome) / "rocksmith-launcher" / "runners";
    }

    const char *home = std::getenv("HOME");
    if (home != nullptr) {
        return fs::path(home) / ".local" / "share" / "rocksmith-launcher" / "runners";
    }

    throw std::runtime_error("Neither XDG_DATA_HOME nor HOME is set");
}

} // namespace rocklaunch
